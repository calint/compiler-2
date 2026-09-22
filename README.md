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
  `address_of`, `equal`, `mov`, `syscall`, `exit`

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
C/C++ Header                    51           2798            862           9696
C++                              1             46              7            231
-------------------------------------------------------------------------------
SUM:                            52           2844            869           9927
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
    cmp_166_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_36_26_166_5:
        cmp_36_26_166_5:
        cmp r15b, 0
        jne if_36_23_166_5_end
        if_36_26_166_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_166_5_end:
    assert_166_5_end:
    mov qword [rbp + 240], -1
    cmp_170_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_36_26_170_5:
        cmp_36_26_170_5:
        cmp r15b, 0
        jne if_36_23_170_5_end
        if_36_26_170_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_170_5_end:
    assert_170_5_end:
        cmp_174_16:
        bool_end_174_16:
        mov r15b, 1
        assert_174_9:
            if_36_26_174_9:
            cmp_36_26_174_9:
            cmp r15b, 0
            jne if_36_23_174_9_end
            if_36_26_174_9_code:
                mov r14, 1
                mov rdi, r14
                mov rax, 60
                syscall
            if_36_23_174_9_end:
        assert_174_9_end:
    cmp_177_12:
    bool_end_177_12:
    mov r15b, 1
    assert_177_5:
        if_36_26_177_5:
        cmp_36_26_177_5:
        cmp r15b, 0
        jne if_36_23_177_5_end
        if_36_26_177_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_177_5_end:
    assert_177_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 183
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_184_12:
    mov r14, 1
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_184_12:
    assert_184_5:
        if_36_26_184_5:
        cmp_36_26_184_5:
        cmp r15b, 0
        jne if_36_23_184_5_end
        if_36_26_184_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_184_5_end:
    assert_184_5_end:
    cmp_185_12:
    mov r14, 2
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_185_12:
    assert_185_5:
        if_36_26_185_5:
        cmp_36_26_185_5:
        cmp r15b, 0
        jne if_36_23_185_5_end
        if_36_26_185_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_185_5_end:
    assert_185_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 187
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 187
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_189_12:
    mov r14, 0
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_189_12:
    assert_189_5:
        if_36_26_189_5:
        cmp_36_26_189_5:
        cmp r15b, 0
        jne if_36_23_189_5_end
        if_36_26_189_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_189_5_end:
    assert_189_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 192
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 192
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_193_12:
        mov rcx, 4
        mov r13, 193
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 193
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
    bool_end_193_12:
    assert_193_5:
        if_36_26_193_5:
        cmp_36_26_193_5:
        cmp r15b, 0
        jne if_36_23_193_5_end
        if_36_26_193_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_193_5_end:
    assert_193_5_end:
    mov r15, 2
    mov r14, 196
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_197_12:
        mov rcx, 4
        mov r13, 197
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 197
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
    bool_end_197_12:
    assert_197_5:
        if_36_26_197_5:
        cmp_36_26_197_5:
        cmp r15b, 0
        jne if_36_23_197_5_end
        if_36_26_197_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_197_5_end:
    assert_197_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 200
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 200
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_200_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_200_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_201_12:
    mov r14, qword [rbp + 248]
    mov r13, 201
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_36_26_201_5:
        cmp_36_26_201_5:
        cmp r15b, 0
        jne if_36_23_201_5_end
        if_36_26_201_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_201_5_end:
    assert_201_5_end:
    faz_203_5:
        mov r15, 1
        mov r14, 88
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_203_5_end:
    cmp_204_12:
    mov r14, 1
    mov r13, 204
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_36_26_204_5:
        cmp_36_26_204_5:
        cmp r15b, 0
        jne if_36_23_204_5_end
        if_36_26_204_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_204_5_end:
    assert_204_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_207_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_207_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_207_5
    foo_207_5_end:
    cmp_210_12:
    mov r14, 0
    mov r13, 210
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
    bool_end_210_12:
    assert_210_5:
        if_36_26_210_5:
        cmp_36_26_210_5:
        cmp r15b, 0
        jne if_36_23_210_5_end
        if_36_26_210_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_210_5_end:
    assert_210_5_end:
    cmp_211_12:
    mov r14, 1
    mov r13, 211
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
    bool_end_211_12:
    assert_211_5:
        if_36_26_211_5:
        cmp_36_26_211_5:
        cmp r15b, 0
        jne if_36_23_211_5_end
        if_36_26_211_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_211_5_end:
    assert_211_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_218_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_218_5_end:
    cmp_219_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_36_26_219_5:
        cmp_36_26_219_5:
        cmp r15b, 0
        jne if_36_23_219_5_end
        if_36_26_219_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_219_5_end:
    assert_219_5_end:
    cmp_220_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_220_12:
    assert_220_5:
        if_36_26_220_5:
        cmp_36_26_220_5:
        cmp r15b, 0
        jne if_36_23_220_5_end
        if_36_26_220_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_220_5_end:
    assert_220_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_223_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_223_12:
    assert_223_5:
        if_36_26_223_5:
        cmp_36_26_223_5:
        cmp r15b, 0
        jne if_36_23_223_5_end
        if_36_26_223_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_223_5_end:
    assert_223_5_end:
    mov qword [rbp + 320], 3
    cmp_228_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_36_26_228_5:
        cmp_36_26_228_5:
        cmp r15b, 0
        jne if_36_23_228_5_end
        if_36_26_228_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_228_5_end:
    assert_228_5_end:
    mov qword [rbp + 336], 0
    bar_231_5:
        if_69_8_231_5:
        cmp_69_8_231_5:
        cmp qword [rbp + 336], 0
        jne if_69_5_231_5_end
        if_69_8_231_5_code:
            jmp bar_231_5_end
        if_69_5_231_5_end:
        mov qword [rbp + 336], 255
    bar_231_5_end:
    cmp_232_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_36_26_232_5:
        cmp_36_26_232_5:
        cmp r15b, 0
        jne if_36_23_232_5_end
        if_36_26_232_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_232_5_end:
    assert_232_5_end:
    mov qword [rbp + 336], 1
    bar_235_5:
        if_69_8_235_5:
        cmp_69_8_235_5:
        cmp qword [rbp + 336], 0
        jne if_69_5_235_5_end
        if_69_8_235_5_code:
            jmp bar_235_5_end
        if_69_5_235_5_end:
        mov qword [rbp + 336], 255
    bar_235_5_end:
    cmp_236_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_36_26_236_5:
        cmp_36_26_236_5:
        cmp r15b, 0
        jne if_36_23_236_5_end
        if_36_26_236_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_236_5_end:
    assert_236_5_end:
    mov qword [rbp + 344], 1
    baz_239_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_239_13_end:
    cmp_240_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_36_26_240_5:
        cmp_36_26_240_5:
        cmp r15b, 0
        jne if_36_23_240_5_end
        if_36_26_240_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_240_5_end:
    assert_240_5_end:
    baz_242_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_242_9_end:
    cmp_243_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_243_12:
    assert_243_5:
        if_36_26_243_5:
        cmp_36_26_243_5:
        cmp r15b, 0
        jne if_36_23_243_5_end
        if_36_26_243_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_243_5_end:
    assert_243_5_end:
    baz_245_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_245_23_end:
    mov qword [rbp + 368], 0
    cmp_246_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_246_12:
    assert_246_5:
        if_36_26_246_5:
        cmp_36_26_246_5:
        cmp r15b, 0
        jne if_36_23_246_5_end
        if_36_26_246_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_246_5_end:
    assert_246_5_end:
    point_init_248_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_248_22_end:
    cmp_249_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_249_12:
    assert_249_5:
        if_36_26_249_5:
        cmp_36_26_249_5:
        cmp r15b, 0
        jne if_36_23_249_5_end
        if_36_26_249_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_249_5_end:
    assert_249_5_end:
    cmp_250_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_250_12:
    assert_250_5:
        if_36_26_250_5:
        cmp_36_26_250_5:
        cmp r15b, 0
        jne if_36_23_250_5_end
        if_36_26_250_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_250_5_end:
    assert_250_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_256_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_36_26_256_5:
        cmp_36_26_256_5:
        cmp r15b, 0
        jne if_36_23_256_5_end
        if_36_26_256_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_256_5_end:
    assert_256_5_end:
    cmp_257_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_257_12:
    assert_257_5:
        if_36_26_257_5:
        cmp_36_26_257_5:
        cmp r15b, 0
        jne if_36_23_257_5_end
        if_36_26_257_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_257_5_end:
    assert_257_5_end:
    cmp_258_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_258_12:
    assert_258_5:
        if_36_26_258_5:
        cmp_36_26_258_5:
        cmp r15b, 0
        jne if_36_23_258_5_end
        if_36_26_258_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_258_5_end:
    assert_258_5_end:
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
    cmp_262_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_262_12:
    assert_262_5:
        if_36_26_262_5:
        cmp_36_26_262_5:
        cmp r15b, 0
        jne if_36_23_262_5_end
        if_36_26_262_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_262_5_end:
    assert_262_5_end:
    cmp_263_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_36_26_263_5:
        cmp_36_26_263_5:
        cmp r15b, 0
        jne if_36_23_263_5_end
        if_36_26_263_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_263_5_end:
    assert_263_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_266_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_266_12:
    assert_266_5:
        if_36_26_266_5:
        cmp_36_26_266_5:
        cmp r15b, 0
        jne if_36_23_266_5_end
        if_36_26_266_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_266_5_end:
    assert_266_5_end:
    cmp_267_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_267_12:
    assert_267_5:
        if_36_26_267_5:
        cmp_36_26_267_5:
        cmp r15b, 0
        jne if_36_23_267_5_end
        if_36_26_267_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_267_5_end:
    assert_267_5_end:
    cmp_268_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_36_26_268_5:
        cmp_36_26_268_5:
        cmp r15b, 0
        jne if_36_23_268_5_end
        if_36_26_268_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_268_5_end:
    assert_268_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_274_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 274
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
    bool_end_274_12:
    assert_274_5:
        if_36_26_274_5:
        cmp_36_26_274_5:
        cmp r15b, 0
        jne if_36_23_274_5_end
        if_36_26_274_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_274_5_end:
    assert_274_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_276_13:
        mov qword [r15 + 8], 74
    object_init_276_13_end:
    cmp_277_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 277
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
    bool_end_277_12:
    assert_277_5:
        if_36_26_277_5:
        cmp_36_26_277_5:
        cmp r15b, 0
        jne if_36_23_277_5_end
        if_36_26_277_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_277_5_end:
    assert_277_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_281_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 281
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 281
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_281_12:
    assert_281_5:
        if_36_26_281_5:
        cmp_36_26_281_5:
        cmp r15b, 0
        jne if_36_23_281_5_end
        if_36_26_281_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_281_5_end:
    assert_281_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 284
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 284
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 285
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 285
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_291_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 291
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 291
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_291_12:
    assert_291_5:
        if_36_26_291_5:
        cmp_36_26_291_5:
        cmp r15b, 0
        jne if_36_23_291_5_end
        if_36_26_291_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_291_5_end:
    assert_291_5_end:
    cmp_292_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r11, 293
        test r12, r12
        cmovs rbp, r11
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r11
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 293
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r11, 294
        test r12, r12
        cmovs rbp, r11
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r11
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 294
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
    bool_end_292_12:
    assert_292_5:
        if_36_26_292_5:
        cmp_36_26_292_5:
        cmp r15b, 0
        jne if_36_23_292_5_end
        if_36_26_292_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_292_5_end:
    assert_292_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_299_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_299_12:
    assert_299_5:
        if_36_26_299_5:
        cmp_36_26_299_5:
        cmp r15b, 0
        jne if_36_23_299_5_end
        if_36_26_299_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_299_5_end:
    assert_299_5_end:
    cmp_300_12:
    mov r14, 0
    mov r13, 300
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_300_12:
    assert_300_5:
        if_36_26_300_5:
        cmp_36_26_300_5:
        cmp r15b, 0
        jne if_36_23_300_5_end
        if_36_26_300_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_300_5_end:
    assert_300_5_end:
    cmp_301_12:
    mov r14, 1
    mov r13, 301
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_301_12:
    assert_301_5:
        if_36_26_301_5:
        cmp_36_26_301_5:
        cmp r15b, 0
        jne if_36_23_301_5_end
        if_36_26_301_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_301_5_end:
    assert_301_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_305_5:
        mov rdx, 21
        lea rsi, [rbp]
        sys_print_47_4_305_5:
                mov rax, 1
                mov rdi, 0
            push r11
            syscall
            pop r11
        sys_print_47_4_305_5_end:
    print_305_5_end:
    loop_306_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_309_9:
            mov rdx, 2
            lea rsi, [rbp + 61]
            sys_print_47_4_309_9:
                    mov rax, 1
                    mov rdi, 0
                push r11
                syscall
                pop r11
            sys_print_47_4_309_9_end:
        print_309_9_end:
        print_310_9:
            mov rdx, 12
            lea rsi, [rbp + 21]
            sys_print_47_4_310_9:
                    mov rax, 1
                    mov rdi, 0
                push r11
                syscall
                pop r11
            sys_print_47_4_310_9_end:
        print_310_9_end:
        str_in_311_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rbp + 1041]
                mov rdx, 127
            push r11
            syscall
            pop r11
                mov byte [rbp + 1040], al
                sub byte [rbp + 1040], 1
        str_in_311_9_end:
        if_312_12:
        cmp_312_12:
        cmp byte [rbp + 1040], 0
        jne if_314_19
        if_312_12_code:
            jmp loop_306_5_end
        jmp if_312_9_end
        if_314_19:
        cmp_314_19:
        cmp byte [rbp + 1040], 4
        jg if_else_312_9
        if_314_19_code:
            print_315_13:
                mov rdx, 20
                lea rsi, [rbp + 33]
                sys_print_47_4_315_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_315_13_end:
            print_315_13_end:
            jmp loop_306_5
        jmp if_312_9_end
        if_else_312_9:
            print_318_13:
                mov rdx, 6
                lea rsi, [rbp + 53]
                sys_print_47_4_318_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_318_13_end:
            print_318_13_end:
            str_out_319_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rbp + 1041]
                    movsx rdx, byte [rbp + 1040]
                push r11
                syscall
                pop r11
            str_out_319_13_end:
            print_320_13:
                mov rdx, 1
                lea rsi, [rbp + 59]
                sys_print_47_4_320_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_320_13_end:
            print_320_13_end:
            print_321_13:
                mov rdx, 1
                lea rsi, [rbp + 60]
                sys_print_47_4_321_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_321_13_end:
            print_321_13_end:
        if_312_9_end:
    jmp loop_306_5
    loop_306_5_end:
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
    if_130_8:
    cmp_130_8:
    cmp qword [rbx + 28], 0
    jge if_130_5_end
    if_130_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_130_5_end:
    mov qword [rbx + 37], 20
    loop_136_5:
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
        mov r14, 140
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
        if_142_12:
        cmp_142_12:
        cmp qword [rbx + 28], 0
        jne if_142_9_end
        if_142_12_code:
            jmp loop_136_5_end
        if_142_9_end:
    jmp loop_136_5
    loop_136_5_end:
    if_145_8:
    cmp_145_8:
    cmp byte [rbx + 36], 0
    je if_145_5_end
    if_145_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 147
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_145_5_end:
    mov qword [rbx + 45], 0
    loop_151_5:
        mov r15, qword [rbx + 45]
        mov r14, 152
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 152
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
        if_155_12:
        cmp_155_12:
        cmp qword [rbx + 37], 20
        jne if_155_9_end
        if_155_12_code:
            jmp loop_151_5_end
        if_155_9_end:
    jmp loop_151_5
    loop_151_5_end:
    mov rdx, qword [rbx + 45]
    lea rsi, [rbx + 8]
    sys_print_158_5:
            mov rax, 1
            mov rdi, 0
        push r11
        syscall
        pop r11
    sys_print_158_5_end:
    ret
print_num.size equ 53
baz_bounds_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    mov rdi, strict qword num_buffer + 19
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
    mov rdx, strict qword num_buffer + 20
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
;[34:1] # single statement blocks can ommit { ... }
;[59:1] # function arguments and return are equivalent to mutable references
;[66:1] # default argument type is `i64`
;[73:1] # return target is specified as a variable, in this case `res`
;[75:1] # return variable is a mutable reference to destination
;[85:1] # array arguments are declared with type and []
;[117:7] const yes = 1
;[118:7] const no = 0
;[119:7] const maybe = -1
;[121:1] # constants can be declared in any scope and shadow outer declarations
;[123:1] # limited support for non-inlined functions
; 
main:
;   [162:5] var arr : i32[4]
;   [162:9] arr: i32[4] (16 B @ [rbp + 224])
;   [162:9] zero 4 * 4 B = 16 B
;   [162:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [163:5] # arrays are initialized to 0
;   [165:5] var answer
;   [165:9] answer: i64 (8 B @ [rbp + 240])
;   [165:9] zero 1 * 8 B = 8 B
;   [165:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [166:5] assert(answer == 0)
;   [166:12] allocate scratch register -> r15
;   [166:12] ? answer == 0
;   [166:12] ? answer == 0
    cmp_166_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_166_12:
;   [36:6] assert(x : bool)
    assert_166_5:
;       [166:5] alias x -> r15b
        if_36_26_166_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_166_5:
        cmp r15b, 0
        jne if_36_23_166_5_end
        if_36_26_166_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_166_5_end:
;       [166:5] free scratch register r15
    assert_166_5_end:
;   [167:5] # variables without initializer are zeroed
;   [169:5] answer = maybe
;   [169:14] maybe
    mov qword [rbp + 240], -1
;   [170:5] assert(answer == -1)
;   [170:12] allocate scratch register -> r15
;   [170:12] ? answer == -1
;   [170:12] ? answer == -1
    cmp_170_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_170_12:
;   [36:6] assert(x : bool)
    assert_170_5:
;       [170:5] alias x -> r15b
        if_36_26_170_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_170_5:
        cmp r15b, 0
        jne if_36_23_170_5_end
        if_36_26_170_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_170_5_end:
;       [170:5] free scratch register r15
    assert_170_5_end:
;       [173:15] const maybe = 33
;       [174:9] assert(maybe == 33)
;       [174:16] allocate scratch register -> r15
;       [174:16] ? maybe == 33
;       [174:16] ? maybe == 33
        cmp_174_16:
;       [174:16] const eval to true
        bool_end_174_16:
        mov r15b, 1
;       [36:6] assert(x : bool)
        assert_174_9:
;           [174:9] alias x -> r15b
            if_36_26_174_9:
;           [36:26] ? not x
;           [36:26] ? not x
            cmp_36_26_174_9:
            cmp r15b, 0
            jne if_36_23_174_9_end
            if_36_26_174_9_code:
;               [36:32] exit(1)
;               [36:32] allocate scratch register -> r14
;               [36:37] 1
                mov r14, 1
                mov rdi, r14
                mov rax, 60
                syscall
;               [36:32] free scratch register r14
            if_36_23_174_9_end:
;           [174:9] free scratch register r15
        assert_174_9_end:
;   [177:5] assert(maybe == -1)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? maybe == -1
;   [177:12] ? maybe == -1
    cmp_177_12:
;   [177:12] const eval to true
    bool_end_177_12:
    mov r15b, 1
;   [36:6] assert(x : bool)
    assert_177_5:
;       [177:5] alias x -> r15b
        if_36_26_177_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_177_5:
        cmp r15b, 0
        jne if_36_23_177_5_end
        if_36_26_177_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_177_5_end:
;       [177:5] free scratch register r15
    assert_177_5_end:
;   [179:5] var ix = 1
;   [179:9] ix: i64 (8 B @ [rbp + 248])
;   [179:9] ix = 1
;   [179:14] 1
    mov qword [rbp + 248], 1
;   [180:5] # variables can have an initial value that can be an expression
;   [182:5] arr[ix] = 2
;   [182:5] allocate scratch register -> r15
;   [182:9] set array index
;   [182:9] ix
    mov r15, qword [rbp + 248]
;   [182:9] bounds check
;   [182:9] allocate scratch register -> r14
;   [182:9] line number
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [182:9] free scratch register r14
;   [182:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [182:5] free scratch register r15
;   [183:5] arr[ix + 1] = arr[ix]
;   [183:5] allocate scratch register -> r15
;   [183:9] set array index
;   [183:9] ix
    mov r15, qword [rbp + 248]
;   [183:14] r15 + 1
    add r15, 1
;   [183:9] bounds check
;   [183:9] allocate scratch register -> r14
;   [183:9] line number
    mov r14, 183
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [183:9] free scratch register r14
;   [183:19] arr[ix]
;   [183:19] allocate scratch register -> r14
;   [183:23] set array index
;   [183:23] ix
    mov r14, qword [rbp + 248]
;   [183:23] bounds check
;   [183:23] allocate scratch register -> r13
;   [183:23] line number
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [183:23] free scratch register r13
;   [183:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [183:19] free scratch register r13
;   [183:19] free scratch register r14
;   [183:5] free scratch register r15
;   [184:5] assert(arr[1] == 2)
;   [184:12] allocate scratch register -> r15
;   [184:12] ? arr[1] == 2
;   [184:12] ? arr[1] == 2
    cmp_184_12:
;   [184:12] allocate scratch register -> r14
;   [184:16] set array index
;   [184:16] 1
    mov r14, 1
;   [184:16] bounds check
;   [184:16] allocate scratch register -> r13
;   [184:16] line number
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [184:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [184:12] free scratch register r14
    sete r15b
    bool_end_184_12:
;   [36:6] assert(x : bool)
    assert_184_5:
;       [184:5] alias x -> r15b
        if_36_26_184_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_184_5:
        cmp r15b, 0
        jne if_36_23_184_5_end
        if_36_26_184_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_184_5_end:
;       [184:5] free scratch register r15
    assert_184_5_end:
;   [185:5] assert(arr[2] == 2)
;   [185:12] allocate scratch register -> r15
;   [185:12] ? arr[2] == 2
;   [185:12] ? arr[2] == 2
    cmp_185_12:
;   [185:12] allocate scratch register -> r14
;   [185:16] set array index
;   [185:16] 2
    mov r14, 2
;   [185:16] bounds check
;   [185:16] allocate scratch register -> r13
;   [185:16] line number
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [185:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [185:12] free scratch register r14
    sete r15b
    bool_end_185_12:
;   [36:6] assert(x : bool)
    assert_185_5:
;       [185:5] alias x -> r15b
        if_36_26_185_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_185_5:
        cmp r15b, 0
        jne if_36_23_185_5_end
        if_36_26_185_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_185_5_end:
;       [185:5] free scratch register r15
    assert_185_5_end:
;   [187:5] array_copy(arr[2], arr, 2)
;   [187:5] allocate named register rsi
;   [187:5] allocate named register rdi
;   [187:5] allocate named register rcx
;   [187:29] 2
;   [187:29] 2
    mov rcx, 2
;   [187:16] arr[2]
;   [187:16] allocate scratch register -> r15
;   [187:20] set array index
;   [187:20] 2
    mov r15, 2
;   [187:20] bounds check
;   [187:20] allocate scratch register -> r14
;   [187:20] line number
    mov r14, 187
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [187:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [187:20] free scratch register r13
    cmovg rbp, r14
    jg baz_bounds_panic
;   [187:20] free scratch register r14
    lea rsi, [rbp + r15 * 4 + 224]
;   [187:5] free scratch register r15
;   [187:24] arr
;   [187:24] bounds check
;   [187:24] allocate scratch register -> r15
;   [187:24] line number
    mov r15, 187
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [187:24] free scratch register r15
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
;   [187:5] free named register rcx
;   [187:5] free named register rdi
;   [187:5] free named register rsi
;   [188:5] # copy from, to, number of elements
;   [189:5] assert(arr[0] == 2)
;   [189:12] allocate scratch register -> r15
;   [189:12] ? arr[0] == 2
;   [189:12] ? arr[0] == 2
    cmp_189_12:
;   [189:12] allocate scratch register -> r14
;   [189:16] set array index
;   [189:16] 0
    mov r14, 0
;   [189:16] bounds check
;   [189:16] allocate scratch register -> r13
;   [189:16] line number
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [189:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [189:12] free scratch register r14
    sete r15b
    bool_end_189_12:
;   [36:6] assert(x : bool)
    assert_189_5:
;       [189:5] alias x -> r15b
        if_36_26_189_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_189_5:
        cmp r15b, 0
        jne if_36_23_189_5_end
        if_36_26_189_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_189_5_end:
;       [189:5] free scratch register r15
    assert_189_5_end:
;   [191:5] var arr1 : i32[8]
;   [191:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [191:9] zero 8 * 4 B = 32 B
;   [191:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [192:5] array_copy(arr, arr1, 4)
;   [192:5] allocate named register rsi
;   [192:5] allocate named register rdi
;   [192:5] allocate named register rcx
;   [192:27] 4
;   [192:27] 4
    mov rcx, 4
;   [192:16] arr
;   [192:16] bounds check
;   [192:16] allocate scratch register -> r15
;   [192:16] line number
    mov r15, 192
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [192:16] free scratch register r15
    lea rsi, [rbp + 224]
;   [192:21] arr1
;   [192:21] bounds check
;   [192:21] allocate scratch register -> r15
;   [192:21] line number
    mov r15, 192
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [192:21] free scratch register r15
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
;   [192:5] free named register rcx
;   [192:5] free named register rdi
;   [192:5] free named register rsi
;   [193:5] assert(arrays_equal(arr, arr1, 4))
;   [193:12] allocate scratch register -> r15
;   [193:12] ? arrays_equal(arr, arr1, 4)
;   [193:12] ? arrays_equal(arr, arr1, 4)
    cmp_193_12:
;   [193:12] allocate scratch register -> r14
;       [193:12] r14b = arrays_equal(arr, arr1, 4)
;       [193:12] = expression
;       [193:12] arrays_equal(arr, arr1, 4)
;       [193:12] allocate named register rsi
;       [193:12] allocate named register rdi
;       [193:12] allocate named register rcx
;       [193:36] 4
;       [193:36] 4
        mov rcx, 4
;       [193:25] arr
;       [193:25] bounds check
;       [193:25] allocate scratch register -> r13
;       [193:25] line number
        mov r13, 193
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [193:25] free scratch register r13
        lea rsi, [rbp + 224]
;       [193:30] arr1
;       [193:30] bounds check
;       [193:30] allocate scratch register -> r13
;       [193:30] line number
        mov r13, 193
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [193:30] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [193:12] free named register rcx
;       [193:12] free named register rdi
;       [193:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [193:12] free scratch register r14
    setne r15b
    bool_end_193_12:
;   [36:6] assert(x : bool)
    assert_193_5:
;       [193:5] alias x -> r15b
        if_36_26_193_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_193_5:
        cmp r15b, 0
        jne if_36_23_193_5_end
        if_36_26_193_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_193_5_end:
;       [193:5] free scratch register r15
    assert_193_5_end:
;   [194:5] # `arrays_equal` is built-in function
;   [196:5] arr1[2] = -1
;   [196:5] allocate scratch register -> r15
;   [196:10] set array index
;   [196:10] 2
    mov r15, 2
;   [196:10] bounds check
;   [196:10] allocate scratch register -> r14
;   [196:10] line number
    mov r14, 196
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [196:10] free scratch register r14
;   [196:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [196:5] free scratch register r15
;   [197:5] assert(not arrays_equal(arr, arr1, 4))
;   [197:12] allocate scratch register -> r15
;   [197:12] ? not arrays_equal(arr, arr1, 4)
;   [197:12] ? not arrays_equal(arr, arr1, 4)
    cmp_197_12:
;   [197:16] allocate scratch register -> r14
;       [197:16] r14b = arrays_equal(arr, arr1, 4)
;       [197:16] = expression
;       [197:16] arrays_equal(arr, arr1, 4)
;       [197:16] allocate named register rsi
;       [197:16] allocate named register rdi
;       [197:16] allocate named register rcx
;       [197:40] 4
;       [197:40] 4
        mov rcx, 4
;       [197:29] arr
;       [197:29] bounds check
;       [197:29] allocate scratch register -> r13
;       [197:29] line number
        mov r13, 197
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [197:29] free scratch register r13
        lea rsi, [rbp + 224]
;       [197:34] arr1
;       [197:34] bounds check
;       [197:34] allocate scratch register -> r13
;       [197:34] line number
        mov r13, 197
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [197:34] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [197:16] free named register rcx
;       [197:16] free named register rdi
;       [197:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [197:12] free scratch register r14
    sete r15b
    bool_end_197_12:
;   [36:6] assert(x : bool)
    assert_197_5:
;       [197:5] alias x -> r15b
        if_36_26_197_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_197_5:
        cmp r15b, 0
        jne if_36_23_197_5_end
        if_36_26_197_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_197_5_end:
;       [197:5] free scratch register r15
    assert_197_5_end:
;   [199:5] ix = 3
;   [199:10] 3
    mov qword [rbp + 248], 3
;   [200:5] arr[ix] = ~inv(arr[ix - 1])
;   [200:5] allocate scratch register -> r15
;   [200:9] set array index
;   [200:9] ix
    mov r15, qword [rbp + 248]
;   [200:9] bounds check
;   [200:9] allocate scratch register -> r14
;   [200:9] line number
    mov r14, 200
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [200:9] free scratch register r14
;   [200:16] arr = ~inv(arr[ix - 1])
;   [200:16] = expression
;   [200:16] ~inv(arr[ix - 1])
;   [200:20] allocate scratch register -> r14
;   [200:24] set array index
;   [200:24] ix
    mov r14, qword [rbp + 248]
;   [200:29] r14 - 1
    sub r14, 1
;   [200:24] bounds check
;   [200:24] allocate scratch register -> r13
;   [200:24] line number
    mov r13, 200
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [200:24] free scratch register r13
;   [77:6] inv(i : i32) : i32 res
    inv_200_16:
;       [200:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [200:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [78:5] res = ~i
;       [78:12] ~i
;       [78:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [78:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [200:16] free scratch register r14
    inv_200_16_end:
    not dword [rbp + r15 * 4 + 224]
;   [200:5] free scratch register r15
;   [201:5] assert(arr[ix] == 2)
;   [201:12] allocate scratch register -> r15
;   [201:12] ? arr[ix] == 2
;   [201:12] ? arr[ix] == 2
    cmp_201_12:
;   [201:12] allocate scratch register -> r14
;   [201:16] set array index
;   [201:16] ix
    mov r14, qword [rbp + 248]
;   [201:16] bounds check
;   [201:16] allocate scratch register -> r13
;   [201:16] line number
    mov r13, 201
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [201:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [201:12] free scratch register r14
    sete r15b
    bool_end_201_12:
;   [36:6] assert(x : bool)
    assert_201_5:
;       [201:5] alias x -> r15b
        if_36_26_201_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_201_5:
        cmp r15b, 0
        jne if_36_23_201_5_end
        if_36_26_201_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_201_5_end:
;       [201:5] free scratch register r15
    assert_201_5_end:
;   [203:5] faz(arr)
;   [87:6] faz(arg : i32[])
    faz_203_5:
;       [203:5] alias arg -> arr
;       [88:5] arg[1] = 0xfe
;       [88:5] allocate scratch register -> r15
;       [88:9] set array index
;       [88:9] 1
        mov r15, 1
;       [88:9] bounds check
;       [88:9] allocate scratch register -> r14
;       [88:9] line number
        mov r14, 88
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
;       [88:9] free scratch register r14
;       [88:14] 0xfe
        mov dword [rbp + r15 * 4 + 224], 254
;       [88:5] free scratch register r15
    faz_203_5_end:
;   [204:5] assert(arr[1] == 0xfe)
;   [204:12] allocate scratch register -> r15
;   [204:12] ? arr[1] == 0xfe
;   [204:12] ? arr[1] == 0xfe
    cmp_204_12:
;   [204:12] allocate scratch register -> r14
;   [204:16] set array index
;   [204:16] 1
    mov r14, 1
;   [204:16] bounds check
;   [204:16] allocate scratch register -> r13
;   [204:16] line number
    mov r13, 204
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [204:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 254
;   [204:12] free scratch register r14
    sete r15b
    bool_end_204_12:
;   [36:6] assert(x : bool)
    assert_204_5:
;       [204:5] alias x -> r15b
        if_36_26_204_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_204_5:
        cmp r15b, 0
        jne if_36_23_204_5_end
        if_36_26_204_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_204_5_end:
;       [204:5] free scratch register r15
    assert_204_5_end:
;   [206:5] var arr3 : i64[] = { 3, 5 }
;   [206:9] arr3: i64[2] (16 B @ [rbp + 288])
;   [206:9] arr3 = { 3, 5 }
;   [206:26] [0]
;   [206:26] 3
    mov qword [rbp + 288], 3
;   [206:26] [1]
;   [206:29] 5
    mov qword [rbp + 296], 5
;   [207:5] foo arr3
;   [207:9] allocate scratch register -> r15
;   [207:9] e: i64 (r15)
;   [207:9] i: i64 (8 B @ [rbp + 312])
;   [207:9] const n = 2
;   [207:9] initiate iterator e
    lea r15, [rbp + 288]
;   [207:9] initiate counter i
    mov qword [rbp + 312], 0
    foo_207_5:
;       [208:9] e = e + i + n
;       [208:13] instructions without scratch register 3, with 4
;       [208:13] e
;       [208:17] e + i
;       [208:17] allocate scratch register -> r14
        mov r14, qword [rbp + 312]
        add qword [r15], r14
;       [208:17] free scratch register r14
;       [208:21] e + n
        add qword [r15], 2
        foo_207_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_207_5
    foo_207_5_end:
;   [207:5] free scratch register r15
;   [210:5] assert(arr3[0] == 3 + 0 + 2)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? arr3[0] == 3 + 0 + 2
;   [210:12] ? arr3[0] == 3 + 0 + 2
    cmp_210_12:
;   [210:12] allocate scratch register -> r14
;   [210:17] set array index
;   [210:17] 0
    mov r14, 0
;   [210:17] bounds check
;   [210:17] allocate scratch register -> r13
;   [210:17] line number
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [210:17] free scratch register r13
;   [210:23] allocate scratch register -> r13
;       [210:23] 3
        mov r13, 3
;       [210:27] r13 + 0
        add r13, 0
;       [210:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [210:12] free scratch register r13
;   [210:12] free scratch register r14
    sete r15b
    bool_end_210_12:
;   [36:6] assert(x : bool)
    assert_210_5:
;       [210:5] alias x -> r15b
        if_36_26_210_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_210_5:
        cmp r15b, 0
        jne if_36_23_210_5_end
        if_36_26_210_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_210_5_end:
;       [210:5] free scratch register r15
    assert_210_5_end:
;   [211:5] assert(arr3[1] == 5 + 1 + 2)
;   [211:12] allocate scratch register -> r15
;   [211:12] ? arr3[1] == 5 + 1 + 2
;   [211:12] ? arr3[1] == 5 + 1 + 2
    cmp_211_12:
;   [211:12] allocate scratch register -> r14
;   [211:17] set array index
;   [211:17] 1
    mov r14, 1
;   [211:17] bounds check
;   [211:17] allocate scratch register -> r13
;   [211:17] line number
    mov r13, 211
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [211:17] free scratch register r13
;   [211:23] allocate scratch register -> r13
;       [211:23] 5
        mov r13, 5
;       [211:27] r13 + 1
        add r13, 1
;       [211:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [211:12] free scratch register r13
;   [211:12] free scratch register r14
    sete r15b
    bool_end_211_12:
;   [36:6] assert(x : bool)
    assert_211_5:
;       [211:5] alias x -> r15b
        if_36_26_211_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_211_5:
        cmp r15b, 0
        jne if_36_23_211_5_end
        if_36_26_211_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_211_5_end:
;       [211:5] free scratch register r15
    assert_211_5_end:
;   [212:5] # `foo` is a language construct that iterates over an array injecting:
;   [213:5] # `e`: current element
;   [214:5] # `i`: index starting at 0
;   [215:5] # `n`: constant array size
;   [217:5] var p : point = {0, 0}
;   [217:9] p: point (16 B @ [rbp + 304])
;   [217:9] p = {0, 0}
;   [217:22] copy field 'x'
    mov qword [rbp + 304], 0
;   [217:25] copy field 'y'
    mov qword [rbp + 312], 0
;   [218:5] fooz(p)
;   [61:6] fooz(pt : point)
    fooz_218_5:
;       [218:5] alias pt -> p
;       [62:5] pt.x = 0b10
;       [62:12] 0b10
        mov qword [rbp + 304], 2
;       [62:20] # binary value 2
;       [63:5] pt.y = 0xb
;       [63:12] 0xb
        mov qword [rbp + 312], 11
;       [63:20] # hex value 11
    fooz_218_5_end:
;   [219:5] assert(p.x == 2)
;   [219:12] allocate scratch register -> r15
;   [219:12] ? p.x == 2
;   [219:12] ? p.x == 2
    cmp_219_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_219_12:
;   [36:6] assert(x : bool)
    assert_219_5:
;       [219:5] alias x -> r15b
        if_36_26_219_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_219_5:
        cmp r15b, 0
        jne if_36_23_219_5_end
        if_36_26_219_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_219_5_end:
;       [219:5] free scratch register r15
    assert_219_5_end:
;   [220:5] assert(p.y == 0xb)
;   [220:12] allocate scratch register -> r15
;   [220:12] ? p.y == 0xb
;   [220:12] ? p.y == 0xb
    cmp_220_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_220_12:
;   [36:6] assert(x : bool)
    assert_220_5:
;       [220:5] alias x -> r15b
        if_36_26_220_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_220_5:
        cmp r15b, 0
        jne if_36_23_220_5_end
        if_36_26_220_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_220_5_end:
;       [220:5] free scratch register r15
    assert_220_5_end:
;   [222:5] var q : point = p
;   [222:9] q: point (16 B @ [rbp + 320])
;   [222:9] q = p
;   [222:21] size <= 16 B, use mov
;   [222:21] allocate named register rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
;   [222:21] free named register rax
;   [223:5] assert(equal(p, q))
;   [223:12] allocate scratch register -> r15
;   [223:12] ? equal(p, q)
;   [223:12] ? equal(p, q)
    cmp_223_12:
;   [223:12] allocate scratch register -> r14
;       [223:12] r14b = equal(p, q)
;       [223:12] = expression
;       [223:12] equal(p, q)
;       [223:12] allocate named register rsi
;       [223:12] allocate named register rdi
;       [223:12] allocate named register rcx
;       [223:18] p
        lea rsi, [rbp + 304]
;       [223:21] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [223:12] free named register rcx
;       [223:12] free named register rdi
;       [223:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [223:12] free scratch register r14
    setne r15b
    bool_end_223_12:
;   [36:6] assert(x : bool)
    assert_223_5:
;       [223:5] alias x -> r15b
        if_36_26_223_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_223_5:
        cmp r15b, 0
        jne if_36_23_223_5_end
        if_36_26_223_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_223_5_end:
;       [223:5] free scratch register r15
    assert_223_5_end:
;   [224:5] # `equal` is built-in function to compare user types for equality or same
;   [225:5] # size arrays
;   [227:5] q.x = 3
;   [227:11] 3
    mov qword [rbp + 320], 3
;   [228:5] assert(not equal(p, q))
;   [228:12] allocate scratch register -> r15
;   [228:12] ? not equal(p, q)
;   [228:12] ? not equal(p, q)
    cmp_228_12:
;   [228:16] allocate scratch register -> r14
;       [228:16] r14b = equal(p, q)
;       [228:16] = expression
;       [228:16] equal(p, q)
;       [228:16] allocate named register rsi
;       [228:16] allocate named register rdi
;       [228:16] allocate named register rcx
;       [228:22] p
        lea rsi, [rbp + 304]
;       [228:25] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [228:16] free named register rcx
;       [228:16] free named register rdi
;       [228:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [228:12] free scratch register r14
    sete r15b
    bool_end_228_12:
;   [36:6] assert(x : bool)
    assert_228_5:
;       [228:5] alias x -> r15b
        if_36_26_228_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_228_5:
        cmp r15b, 0
        jne if_36_23_228_5_end
        if_36_26_228_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_228_5_end:
;       [228:5] free scratch register r15
    assert_228_5_end:
;   [230:5] var i = 0
;   [230:9] i: i64 (8 B @ [rbp + 336])
;   [230:9] i = 0
;   [230:13] 0
    mov qword [rbp + 336], 0
;   [231:5] bar(i)
;   [68:6] bar(arg)
    bar_231_5:
;       [231:5] alias arg -> i
        if_69_8_231_5:
;       [69:8] ? arg == 0
;       [69:8] ? arg == 0
        cmp_69_8_231_5:
        cmp qword [rbp + 336], 0
        jne if_69_5_231_5_end
        if_69_8_231_5_code:
;           [69:17] return
            jmp bar_231_5_end
        if_69_5_231_5_end:
;       [70:5] arg = 0xff
;       [70:11] 0xff
        mov qword [rbp + 336], 255
    bar_231_5_end:
;   [232:5] assert(i == 0)
;   [232:12] allocate scratch register -> r15
;   [232:12] ? i == 0
;   [232:12] ? i == 0
    cmp_232_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_232_12:
;   [36:6] assert(x : bool)
    assert_232_5:
;       [232:5] alias x -> r15b
        if_36_26_232_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_232_5:
        cmp r15b, 0
        jne if_36_23_232_5_end
        if_36_26_232_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_232_5_end:
;       [232:5] free scratch register r15
    assert_232_5_end:
;   [234:5] i = 1
;   [234:9] 1
    mov qword [rbp + 336], 1
;   [235:5] bar(i)
;   [68:6] bar(arg)
    bar_235_5:
;       [235:5] alias arg -> i
        if_69_8_235_5:
;       [69:8] ? arg == 0
;       [69:8] ? arg == 0
        cmp_69_8_235_5:
        cmp qword [rbp + 336], 0
        jne if_69_5_235_5_end
        if_69_8_235_5_code:
;           [69:17] return
            jmp bar_235_5_end
        if_69_5_235_5_end:
;       [70:5] arg = 0xff
;       [70:11] 0xff
        mov qword [rbp + 336], 255
    bar_235_5_end:
;   [236:5] assert(i == 0xff)
;   [236:12] allocate scratch register -> r15
;   [236:12] ? i == 0xff
;   [236:12] ? i == 0xff
    cmp_236_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_236_12:
;   [36:6] assert(x : bool)
    assert_236_5:
;       [236:5] alias x -> r15b
        if_36_26_236_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_236_5:
        cmp r15b, 0
        jne if_36_23_236_5_end
        if_36_26_236_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_236_5_end:
;       [236:5] free scratch register r15
    assert_236_5_end:
;   [238:5] var j = 1
;   [238:9] j: i64 (8 B @ [rbp + 344])
;   [238:9] j = 1
;   [238:13] 1
    mov qword [rbp + 344], 1
;   [239:5] var k = baz(j)
;   [239:9] k: i64 (8 B @ [rbp + 352])
;   [239:9] k = baz(j)
;   [239:13] k = baz(j)
;   [239:13] = expression
;   [239:13] baz(j)
;   [81:6] baz(arg) : i64 res
    baz_239_13:
;       [239:13] alias res -> k
;       [239:13] alias arg -> j
;       [82:5] res = arg * 2
;       [82:11] instructions without scratch register 5, with 3
;       [82:11] allocate scratch register -> r15
;       [82:11] arg
        mov r15, qword [rbp + 344]
;       [82:17] r15 * 2
;       [82:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [82:11] free scratch register r15
    baz_239_13_end:
;   [240:5] assert(k == 2)
;   [240:12] allocate scratch register -> r15
;   [240:12] ? k == 2
;   [240:12] ? k == 2
    cmp_240_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_240_12:
;   [36:6] assert(x : bool)
    assert_240_5:
;       [240:5] alias x -> r15b
        if_36_26_240_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_240_5:
        cmp r15b, 0
        jne if_36_23_240_5_end
        if_36_26_240_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_240_5_end:
;       [240:5] free scratch register r15
    assert_240_5_end:
;   [242:5] k = baz(1)
;   [242:9] k = baz(1)
;   [242:9] = expression
;   [242:9] baz(1)
;   [81:6] baz(arg) : i64 res
    baz_242_9:
;       [242:9] alias res -> k
;       [242:9] alias arg -> 1
;       [82:5] res = arg * 2
;       [82:11] instructions without scratch register 4, with 3
;       [82:11] allocate scratch register -> r15
;       [82:11] arg
        mov r15, 1
;       [82:17] r15 * 2
;       [82:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [82:11] free scratch register r15
    baz_242_9_end:
;   [243:5] assert(k == 2)
;   [243:12] allocate scratch register -> r15
;   [243:12] ? k == 2
;   [243:12] ? k == 2
    cmp_243_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_243_12:
;   [36:6] assert(x : bool)
    assert_243_5:
;       [243:5] alias x -> r15b
        if_36_26_243_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_243_5:
        cmp r15b, 0
        jne if_36_23_243_5_end
        if_36_26_243_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_243_5_end:
;       [243:5] free scratch register r15
    assert_243_5_end:
;   [245:5] var p0 : point = {baz(3), 0}
;   [245:9] p0: point (16 B @ [rbp + 360])
;   [245:9] p0 = {baz(3), 0}
;   [245:23] copy field 'x'
;   [245:23] p0.x = baz(3)
;   [245:23] = expression
;   [245:23] baz(3)
;   [81:6] baz(arg) : i64 res
    baz_245_23:
;       [245:23] alias res -> p0.x (lea: rbp + 360)
;       [245:23] alias arg -> 3
;       [82:5] res = arg * 2
;       [82:11] instructions without scratch register 4, with 3
;       [82:11] allocate scratch register -> r15
;       [82:11] arg
        mov r15, 3
;       [82:17] r15 * 2
;       [82:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 360], r15
;       [82:11] free scratch register r15
    baz_245_23_end:
;   [245:31] copy field 'y'
    mov qword [rbp + 368], 0
;   [246:5] assert(p0.x == 6)
;   [246:12] allocate scratch register -> r15
;   [246:12] ? p0.x == 6
;   [246:12] ? p0.x == 6
    cmp_246_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_246_12:
;   [36:6] assert(x : bool)
    assert_246_5:
;       [246:5] alias x -> r15b
        if_36_26_246_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_246_5:
        cmp r15b, 0
        jne if_36_23_246_5_end
        if_36_26_246_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_246_5_end:
;       [246:5] free scratch register r15
    assert_246_5_end:
;   [248:5] var pt : point = point_init()
;   [248:9] pt: point (16 B @ [rbp + 376])
;   [248:9] pt = point_init()
;   [248:22] point_init()
;   [108:6] point_init() : point res
    point_init_248_22:
;       [248:22] alias res -> pt
;       [109:5] res.x = -1
;       [109:14] -1
        mov qword [rbp + 376], -1
;       [110:5] res.y = -2
;       [110:14] -2
        mov qword [rbp + 384], -2
    point_init_248_22_end:
;   [249:5] assert(pt.x == -1)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? pt.x == -1
;   [249:12] ? pt.x == -1
    cmp_249_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_249_12:
;   [36:6] assert(x : bool)
    assert_249_5:
;       [249:5] alias x -> r15b
        if_36_26_249_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_249_5:
        cmp r15b, 0
        jne if_36_23_249_5_end
        if_36_26_249_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_249_5_end:
;       [249:5] free scratch register r15
    assert_249_5_end:
;   [250:5] assert(pt.y == -2)
;   [250:12] allocate scratch register -> r15
;   [250:12] ? pt.y == -2
;   [250:12] ? pt.y == -2
    cmp_250_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_250_12:
;   [36:6] assert(x : bool)
    assert_250_5:
;       [250:5] alias x -> r15b
        if_36_26_250_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_250_5:
        cmp r15b, 0
        jne if_36_23_250_5_end
        if_36_26_250_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_250_5_end:
;       [250:5] free scratch register r15
    assert_250_5_end:
;   [252:5] var x = 1
;   [252:9] x: i64 (8 B @ [rbp + 392])
;   [252:9] x = 1
;   [252:13] 1
    mov qword [rbp + 392], 1
;   [253:5] var y = 2
;   [253:9] y: i64 (8 B @ [rbp + 400])
;   [253:9] y = 2
;   [253:13] 2
    mov qword [rbp + 400], 2
;   [255:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [255:9] o1: object (20 B @ [rbp + 408])
;   [255:9] o1 = {{x * 10, y}, 0xff0000}
;   [255:24] copy field 'pos'
;   [255:25] copy field 'x'
;   [255:25] instructions without scratch register 5, with 3
;   [255:25] allocate scratch register -> r15
;   [255:25] x
    mov r15, qword [rbp + 392]
;   [255:29] r15 * 10
;   [255:29] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 408], r15
;   [255:25] free scratch register r15
;   [255:33] copy field 'y'
;   [255:33] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
;   [255:33] free scratch register r15
;   [255:37] copy field 'color'
    mov dword [rbp + 424], 16711680
;   [256:5] assert(o1.pos.x == 10)
;   [256:12] allocate scratch register -> r15
;   [256:12] ? o1.pos.x == 10
;   [256:12] ? o1.pos.x == 10
    cmp_256_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_256_12:
;   [36:6] assert(x : bool)
    assert_256_5:
;       [256:5] alias x -> r15b
        if_36_26_256_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_256_5:
        cmp r15b, 0
        jne if_36_23_256_5_end
        if_36_26_256_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_256_5_end:
;       [256:5] free scratch register r15
    assert_256_5_end:
;   [257:5] assert(o1.pos.y == 2)
;   [257:12] allocate scratch register -> r15
;   [257:12] ? o1.pos.y == 2
;   [257:12] ? o1.pos.y == 2
    cmp_257_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_257_12:
;   [36:6] assert(x : bool)
    assert_257_5:
;       [257:5] alias x -> r15b
        if_36_26_257_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_257_5:
        cmp r15b, 0
        jne if_36_23_257_5_end
        if_36_26_257_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_257_5_end:
;       [257:5] free scratch register r15
    assert_257_5_end:
;   [258:5] assert(o1.color == 0xff0000)
;   [258:12] allocate scratch register -> r15
;   [258:12] ? o1.color == 0xff0000
;   [258:12] ? o1.color == 0xff0000
    cmp_258_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_258_12:
;   [36:6] assert(x : bool)
    assert_258_5:
;       [258:5] alias x -> r15b
        if_36_26_258_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_258_5:
        cmp r15b, 0
        jne if_36_23_258_5_end
        if_36_26_258_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_258_5_end:
;       [258:5] free scratch register r15
    assert_258_5_end:
;   [260:5] var p1 : point = {-x, -y}
;   [260:9] p1: point (16 B @ [rbp + 428])
;   [260:9] p1 = {-x, -y}
;   [260:23] copy field 'x'
;   [260:23] allocate scratch register -> r15
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
;   [260:23] free scratch register r15
    neg qword [rbp + 428]
;   [260:27] copy field 'y'
;   [260:27] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
;   [260:27] free scratch register r15
    neg qword [rbp + 436]
;   [261:5] o1.pos = p1
;   [261:14] size <= 16 B, use mov
;   [261:14] allocate named register rax
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
;   [261:14] free named register rax
;   [262:5] assert(o1.pos.x == -1)
;   [262:12] allocate scratch register -> r15
;   [262:12] ? o1.pos.x == -1
;   [262:12] ? o1.pos.x == -1
    cmp_262_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_262_12:
;   [36:6] assert(x : bool)
    assert_262_5:
;       [262:5] alias x -> r15b
        if_36_26_262_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_262_5:
        cmp r15b, 0
        jne if_36_23_262_5_end
        if_36_26_262_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_262_5_end:
;       [262:5] free scratch register r15
    assert_262_5_end:
;   [263:5] assert(o1.pos.y == -2)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? o1.pos.y == -2
;   [263:12] ? o1.pos.y == -2
    cmp_263_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_263_12:
;   [36:6] assert(x : bool)
    assert_263_5:
;       [263:5] alias x -> r15b
        if_36_26_263_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_263_5:
        cmp r15b, 0
        jne if_36_23_263_5_end
        if_36_26_263_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_263_5_end:
;       [263:5] free scratch register r15
    assert_263_5_end:
;   [265:5] var o2 : object = o1
;   [265:9] o2: object (20 B @ [rbp + 444])
;   [265:9] o2 = o1
;   [265:23] allocate named register rsi
;   [265:23] allocate named register rdi
;   [265:23] allocate named register rcx
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
;   [265:23] free named register rcx
;   [265:23] free named register rdi
;   [265:23] free named register rsi
;   [266:5] assert(o2.pos.x == -1)
;   [266:12] allocate scratch register -> r15
;   [266:12] ? o2.pos.x == -1
;   [266:12] ? o2.pos.x == -1
    cmp_266_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_266_12:
;   [36:6] assert(x : bool)
    assert_266_5:
;       [266:5] alias x -> r15b
        if_36_26_266_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_266_5:
        cmp r15b, 0
        jne if_36_23_266_5_end
        if_36_26_266_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_266_5_end:
;       [266:5] free scratch register r15
    assert_266_5_end:
;   [267:5] assert(o2.pos.y == -2)
;   [267:12] allocate scratch register -> r15
;   [267:12] ? o2.pos.y == -2
;   [267:12] ? o2.pos.y == -2
    cmp_267_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_267_12:
;   [36:6] assert(x : bool)
    assert_267_5:
;       [267:5] alias x -> r15b
        if_36_26_267_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_267_5:
        cmp r15b, 0
        jne if_36_23_267_5_end
        if_36_26_267_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_267_5_end:
;       [267:5] free scratch register r15
    assert_267_5_end:
;   [268:5] assert(o2.color == 0xff0000)
;   [268:12] allocate scratch register -> r15
;   [268:12] ? o2.color == 0xff0000
;   [268:12] ? o2.color == 0xff0000
    cmp_268_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_268_12:
;   [36:6] assert(x : bool)
    assert_268_5:
;       [268:5] alias x -> r15b
        if_36_26_268_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_268_5:
        cmp r15b, 0
        jne if_36_23_268_5_end
        if_36_26_268_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_268_5_end:
;       [268:5] free scratch register r15
    assert_268_5_end:
;   [270:5] var o3 : object[2]
;   [270:9] o3: object[2] (40 B @ [rbp + 464])
;   [270:9] zero 2 * 20 B = 40 B
;   [270:5] allocate named register rax
;   [270:5] allocate named register rdi
;   [270:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
;   [270:5] free named register rcx
;   [270:5] free named register rdi
;   [270:5] free named register rax
;   [271:5] o3.pos.y = 73
;   [271:16] 73
    mov qword [rbp + 472], 73
;   [272:5] # index 0 in an array can be accessed without array index
;   [274:5] assert(o3[0].pos.y == 73)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? o3[0].pos.y == 73
;   [274:12] ? o3[0].pos.y == 73
    cmp_274_12:
;   [274:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [274:12] allocate scratch register -> r13
;   [274:15] set array index
;   [274:15] 0
    mov r13, 0
;   [274:15] bounds check
;   [274:15] allocate scratch register -> r12
;   [274:15] line number
    mov r12, 274
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [274:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [274:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [274:12] free scratch register r14
    sete r15b
    bool_end_274_12:
;   [36:6] assert(x : bool)
    assert_274_5:
;       [274:5] alias x -> r15b
        if_36_26_274_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_274_5:
        cmp r15b, 0
        jne if_36_23_274_5_end
        if_36_26_274_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_274_5_end:
;       [274:5] free scratch register r15
    assert_274_5_end:
;   [276:5] o3[1] = object_init()
;   [276:5] allocate scratch register -> r15
    lea r15, [rbp + 464]
;   [276:5] allocate scratch register -> r14
;   [276:8] set array index
;   [276:8] 1
    mov r14, 1
;   [276:8] bounds check
;   [276:8] allocate scratch register -> r13
;   [276:8] line number
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [276:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [276:5] free scratch register r14
;   [276:13] object_init()
;   [113:6] object_init() : object res
    object_init_276_13:
;       [276:13] alias res -> o3 (lea: r15)
;       [114:5] res.pos.y = 74
;       [114:17] 74
        mov qword [r15 + 8], 74
    object_init_276_13_end:
;   [276:5] free scratch register r15
;   [277:5] assert(o3[1].pos.y == 74)
;   [277:12] allocate scratch register -> r15
;   [277:12] ? o3[1].pos.y == 74
;   [277:12] ? o3[1].pos.y == 74
    cmp_277_12:
;   [277:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [277:12] allocate scratch register -> r13
;   [277:15] set array index
;   [277:15] 1
    mov r13, 1
;   [277:15] bounds check
;   [277:15] allocate scratch register -> r12
;   [277:15] line number
    mov r12, 277
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [277:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [277:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [277:12] free scratch register r14
    sete r15b
    bool_end_277_12:
;   [36:6] assert(x : bool)
    assert_277_5:
;       [277:5] alias x -> r15b
        if_36_26_277_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_277_5:
        cmp r15b, 0
        jne if_36_23_277_5_end
        if_36_26_277_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_277_5_end:
;       [277:5] free scratch register r15
    assert_277_5_end:
;   [279:5] var worlds : world[8]
;   [279:9] worlds: world[8] (512 B @ [rbp + 504])
;   [279:9] zero 8 * 64 B = 512 B
;   [279:5] allocate named register rax
;   [279:5] allocate named register rdi
;   [279:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
;   [279:5] free named register rcx
;   [279:5] free named register rdi
;   [279:5] free named register rax
;   [280:5] worlds[1].locations[1] = 0xffee
;   [280:5] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [280:5] allocate scratch register -> r14
;   [280:12] set array index
;   [280:12] 1
    mov r14, 1
;   [280:12] bounds check
;   [280:12] allocate scratch register -> r13
;   [280:12] line number
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [280:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [280:5] free scratch register r14
;   [280:5] allocate scratch register -> r14
;   [280:25] set array index
;   [280:25] 1
    mov r14, 1
;   [280:25] bounds check
;   [280:25] allocate scratch register -> r13
;   [280:25] line number
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [280:25] free scratch register r13
;   [280:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [280:5] free scratch register r14
;   [280:5] free scratch register r15
;   [281:5] assert(worlds[1].locations[1] == 0xffee)
;   [281:12] allocate scratch register -> r15
;   [281:12] ? worlds[1].locations[1] == 0xffee
;   [281:12] ? worlds[1].locations[1] == 0xffee
    cmp_281_12:
;   [281:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [281:12] allocate scratch register -> r13
;   [281:19] set array index
;   [281:19] 1
    mov r13, 1
;   [281:19] bounds check
;   [281:19] allocate scratch register -> r12
;   [281:19] line number
    mov r12, 281
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [281:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [281:12] free scratch register r13
;   [281:12] allocate scratch register -> r13
;   [281:32] set array index
;   [281:32] 1
    mov r13, 1
;   [281:32] bounds check
;   [281:32] allocate scratch register -> r12
;   [281:32] line number
    mov r12, 281
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [281:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [281:12] free scratch register r13
;   [281:12] free scratch register r14
    sete r15b
    bool_end_281_12:
;   [36:6] assert(x : bool)
    assert_281_5:
;       [281:5] alias x -> r15b
        if_36_26_281_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_281_5:
        cmp r15b, 0
        jne if_36_23_281_5_end
        if_36_26_281_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_281_5_end:
;       [281:5] free scratch register r15
    assert_281_5_end:
;   [283:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [283:5] allocate named register rsi
;   [283:5] allocate named register rdi
;   [283:5] allocate named register rcx
;   [286:9] array_size_of(worlds.locations)
;   [286:9] rcx = array_size_of(worlds.locations)
;   [286:9] = expression
;   [286:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [284:9] worlds[1].locations
;   [284:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [284:9] allocate scratch register -> r14
;   [284:16] set array index
;   [284:16] 1
    mov r14, 1
;   [284:16] bounds check
;   [284:16] allocate scratch register -> r13
;   [284:16] line number
    mov r13, 284
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [284:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [284:9] free scratch register r14
;   [284:9] bounds check
;   [284:9] allocate scratch register -> r14
;   [284:9] line number
    mov r14, 284
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [284:9] free scratch register r14
    lea rsi, [r15]
;   [283:5] free scratch register r15
;   [285:9] worlds[0].locations
;   [285:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [285:9] allocate scratch register -> r14
;   [285:16] set array index
;   [285:16] 0
    mov r14, 0
;   [285:16] bounds check
;   [285:16] allocate scratch register -> r13
;   [285:16] line number
    mov r13, 285
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [285:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [285:9] free scratch register r14
;   [285:9] bounds check
;   [285:9] allocate scratch register -> r14
;   [285:9] line number
    mov r14, 285
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [285:9] free scratch register r14
    lea rdi, [r15]
;   [283:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [283:5] free named register rcx
;   [283:5] free named register rdi
;   [283:5] free named register rsi
;   [288:5] # `array_copy` is built-in and can use indexed positions
;   [289:5] # `array_size_of` is built-in
;   [291:5] assert(worlds[0].locations[1] == 0xffee)
;   [291:12] allocate scratch register -> r15
;   [291:12] ? worlds[0].locations[1] == 0xffee
;   [291:12] ? worlds[0].locations[1] == 0xffee
    cmp_291_12:
;   [291:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [291:12] allocate scratch register -> r13
;   [291:19] set array index
;   [291:19] 0
    mov r13, 0
;   [291:19] bounds check
;   [291:19] allocate scratch register -> r12
;   [291:19] line number
    mov r12, 291
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [291:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [291:12] free scratch register r13
;   [291:12] allocate scratch register -> r13
;   [291:32] set array index
;   [291:32] 1
    mov r13, 1
;   [291:32] bounds check
;   [291:32] allocate scratch register -> r12
;   [291:32] line number
    mov r12, 291
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [291:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [291:12] free scratch register r13
;   [291:12] free scratch register r14
    sete r15b
    bool_end_291_12:
;   [36:6] assert(x : bool)
    assert_291_5:
;       [291:5] alias x -> r15b
        if_36_26_291_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_291_5:
        cmp r15b, 0
        jne if_36_23_291_5_end
        if_36_26_291_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_291_5_end:
;       [291:5] free scratch register r15
    assert_291_5_end:
;   [292:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [292:12] allocate scratch register -> r15
;   [292:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [292:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_292_12:
;   [292:12] allocate scratch register -> r14
;       [292:12] r14b = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [292:12] = expression
;       [292:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [292:12] allocate named register rsi
;       [292:12] allocate named register rdi
;       [292:12] allocate named register rcx
;       [295:14] array_size_of(worlds.locations)
;       [295:14] rcx = array_size_of(worlds.locations)
;       [295:14] = expression
;       [295:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [293:14] worlds[0].locations
;       [293:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [293:14] allocate scratch register -> r12
;       [293:21] set array index
;       [293:21] 0
        mov r12, 0
;       [293:21] bounds check
;       [293:21] allocate scratch register -> r11
;       [293:21] line number
        mov r11, 293
        test r12, r12
        cmovs rbp, r11
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r11
        jge baz_bounds_panic
;       [293:21] free scratch register r11
        shl r12, 6
        add r13, r12
;       [293:14] free scratch register r12
;       [293:14] bounds check
;       [293:14] allocate scratch register -> r12
;       [293:14] line number
        mov r12, 293
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [293:14] free scratch register r12
        lea rsi, [r13]
;       [292:12] free scratch register r13
;       [294:14] worlds[1].locations
;       [294:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [294:14] allocate scratch register -> r12
;       [294:21] set array index
;       [294:21] 1
        mov r12, 1
;       [294:21] bounds check
;       [294:21] allocate scratch register -> r11
;       [294:21] line number
        mov r11, 294
        test r12, r12
        cmovs rbp, r11
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r11
        jge baz_bounds_panic
;       [294:21] free scratch register r11
        shl r12, 6
        add r13, r12
;       [294:14] free scratch register r12
;       [294:14] bounds check
;       [294:14] allocate scratch register -> r12
;       [294:14] line number
        mov r12, 294
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [294:14] free scratch register r12
        lea rdi, [r13]
;       [292:12] free scratch register r13
        shl rcx, 3
        repe cmpsb
;       [292:12] free named register rcx
;       [292:12] free named register rdi
;       [292:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [292:12] free scratch register r14
    setne r15b
    bool_end_292_12:
;   [36:6] assert(x : bool)
    assert_292_5:
;       [292:5] alias x -> r15b
        if_36_26_292_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_292_5:
        cmp r15b, 0
        jne if_36_23_292_5_end
        if_36_26_292_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_292_5_end:
;       [292:5] free scratch register r15
    assert_292_5_end:
;   [298:5] var arr2 : i64[] = { -1, 2 }
;   [298:9] arr2: i64[2] (16 B @ [rbp + 1016])
;   [298:9] arr2 = { -1, 2 }
;   [298:26] [0]
;   [298:27] -1
    mov qword [rbp + 1016], -1
;   [298:26] [1]
;   [298:30] 2
    mov qword [rbp + 1024], 2
;   [299:5] assert(array_size_of(arr2) == 2)
;   [299:12] allocate scratch register -> r15
;   [299:12] ? array_size_of(arr2) == 2
;   [299:12] ? array_size_of(arr2) == 2
    cmp_299_12:
;   [299:12] allocate scratch register -> r14
;       [299:12] r14 = array_size_of(arr2)
;       [299:12] = expression
;       [299:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [299:12] free scratch register r14
    sete r15b
    bool_end_299_12:
;   [36:6] assert(x : bool)
    assert_299_5:
;       [299:5] alias x -> r15b
        if_36_26_299_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_299_5:
        cmp r15b, 0
        jne if_36_23_299_5_end
        if_36_26_299_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_299_5_end:
;       [299:5] free scratch register r15
    assert_299_5_end:
;   [300:5] assert(arr2[0] == -1)
;   [300:12] allocate scratch register -> r15
;   [300:12] ? arr2[0] == -1
;   [300:12] ? arr2[0] == -1
    cmp_300_12:
;   [300:12] allocate scratch register -> r14
;   [300:17] set array index
;   [300:17] 0
    mov r14, 0
;   [300:17] bounds check
;   [300:17] allocate scratch register -> r13
;   [300:17] line number
    mov r13, 300
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [300:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], -1
;   [300:12] free scratch register r14
    sete r15b
    bool_end_300_12:
;   [36:6] assert(x : bool)
    assert_300_5:
;       [300:5] alias x -> r15b
        if_36_26_300_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_300_5:
        cmp r15b, 0
        jne if_36_23_300_5_end
        if_36_26_300_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_300_5_end:
;       [300:5] free scratch register r15
    assert_300_5_end:
;   [301:5] assert(arr2[1] == 2)
;   [301:12] allocate scratch register -> r15
;   [301:12] ? arr2[1] == 2
;   [301:12] ? arr2[1] == 2
    cmp_301_12:
;   [301:12] allocate scratch register -> r14
;   [301:17] set array index
;   [301:17] 1
    mov r14, 1
;   [301:17] bounds check
;   [301:17] allocate scratch register -> r13
;   [301:17] line number
    mov r13, 301
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [301:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], 2
;   [301:12] free scratch register r14
    sete r15b
    bool_end_301_12:
;   [36:6] assert(x : bool)
    assert_301_5:
;       [301:5] alias x -> r15b
        if_36_26_301_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_301_5:
        cmp r15b, 0
        jne if_36_23_301_5_end
        if_36_26_301_5_code:
;           [36:32] exit(1)
;           [36:32] allocate scratch register -> r14
;           [36:37] 1
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
;           [36:32] free scratch register r14
        if_36_23_301_5_end:
;       [301:5] free scratch register r15
    assert_301_5_end:
;   [303:5] var counter
;   [303:9] counter: i64 (8 B @ [rbp + 1032])
;   [303:9] zero 1 * 8 B = 8 B
;   [303:5] size <= 32 B, use mov
    mov qword [rbp + 1032], 0
;   [304:5] var nm : str
;   [304:9] nm: str (128 B @ [rbp + 1040])
;   [304:9] zero 1 * 128 B = 128 B
;   [304:5] allocate named register rax
;   [304:5] allocate named register rdi
;   [304:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
;   [304:5] free named register rcx
;   [304:5] free named register rdi
;   [304:5] free named register rax
;   [305:5] print(hello)
;   [46:6] print(str : i8[])
    print_305_5:
;       [305:5] alias str -> hello
;       [47:4] sys_print(array_size_of(str), address_of(str))
;       [47:14] allocate named register rdx
;       [47:14] rdx = array_size_of(str)
;       [47:14] = expression
;       [47:14] array_size_of(str)
        mov rdx, 21
;       [47:34] allocate named register rsi
;       [47:34] rsi = address_of(str)
;       [47:34] = expression
;       [47:34] address_of(str)
        lea rsi, [rbp]
;       [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
        sys_print_47_4_305_5:
;           [47:4] alias len -> rdx
;           [47:4] alias ptr -> rsi
;           [39:5] mov(rax, 1)
;               [39:14] 1
                mov rax, 1
;           [39:19] # write system call
;           [40:5] mov(rdi, 0)
;               [40:14] 0
                mov rdi, 0
;           [40:19] # file descriptor for standard out
;           [41:5] mov(rsi, ptr)
;               [41:14] ptr
;           [41:19] # buffer address
;           [42:5] mov(rdx, len)
;               [42:14] len
;           [42:19] # buffer size
;           [43:5] syscall()
            push r11
            syscall
            pop r11
;           [47:4] free named register rsi
;           [47:4] free named register rdx
        sys_print_47_4_305_5_end:
    print_305_5_end:
;   [306:5] label
    loop_306_5:
;       [307:9] counter = counter + 1
;       [307:19] instructions without scratch register 1, with 3
;       [307:19] counter
;       [307:29] counter + 1
        add qword [rbp + 1032], 1
;       [308:9] print_num(counter)
;       [308:9] address of argument 'counter' to parameter 'num'
;       [308:9] allocate scratch register -> r15
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
;       [308:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
;       [309:9] print(colon)
;       [46:6] print(str : i8[])
        print_309_9:
;           [309:9] alias str -> colon
;           [47:4] sys_print(array_size_of(str), address_of(str))
;           [47:14] allocate named register rdx
;           [47:14] rdx = array_size_of(str)
;           [47:14] = expression
;           [47:14] array_size_of(str)
            mov rdx, 2
;           [47:34] allocate named register rsi
;           [47:34] rsi = address_of(str)
;           [47:34] = expression
;           [47:34] address_of(str)
            lea rsi, [rbp + 61]
;           [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
            sys_print_47_4_309_9:
;               [47:4] alias len -> rdx
;               [47:4] alias ptr -> rsi
;               [39:5] mov(rax, 1)
;                   [39:14] 1
                    mov rax, 1
;               [39:19] # write system call
;               [40:5] mov(rdi, 0)
;                   [40:14] 0
                    mov rdi, 0
;               [40:19] # file descriptor for standard out
;               [41:5] mov(rsi, ptr)
;                   [41:14] ptr
;               [41:19] # buffer address
;               [42:5] mov(rdx, len)
;                   [42:14] len
;               [42:19] # buffer size
;               [43:5] syscall()
                push r11
                syscall
                pop r11
;               [47:4] free named register rsi
;               [47:4] free named register rdx
            sys_print_47_4_309_9_end:
        print_309_9_end:
;       [310:9] print(prompt1)
;       [46:6] print(str : i8[])
        print_310_9:
;           [310:9] alias str -> prompt1
;           [47:4] sys_print(array_size_of(str), address_of(str))
;           [47:14] allocate named register rdx
;           [47:14] rdx = array_size_of(str)
;           [47:14] = expression
;           [47:14] array_size_of(str)
            mov rdx, 12
;           [47:34] allocate named register rsi
;           [47:34] rsi = address_of(str)
;           [47:34] = expression
;           [47:34] address_of(str)
            lea rsi, [rbp + 21]
;           [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
            sys_print_47_4_310_9:
;               [47:4] alias len -> rdx
;               [47:4] alias ptr -> rsi
;               [39:5] mov(rax, 1)
;                   [39:14] 1
                    mov rax, 1
;               [39:19] # write system call
;               [40:5] mov(rdi, 0)
;                   [40:14] 0
                    mov rdi, 0
;               [40:19] # file descriptor for standard out
;               [41:5] mov(rsi, ptr)
;                   [41:14] ptr
;               [41:19] # buffer address
;               [42:5] mov(rdx, len)
;                   [42:14] len
;               [42:19] # buffer size
;               [43:5] syscall()
                push r11
                syscall
                pop r11
;               [47:4] free named register rsi
;               [47:4] free named register rdx
            sys_print_47_4_310_9_end:
        print_310_9_end:
;       [311:9] str_in(nm)
;       [91:6] str_in(s : str)
        str_in_311_9:
;           [311:9] alias s -> nm
;           [92:5] mov(rax, 0)
;               [92:14] 0
                mov rax, 0
;           [92:37] # read system call
;           [93:5] mov(rdi, 0)
;               [93:14] 0
                mov rdi, 0
;           [93:37] # file descriptor for standard input
;           [94:5] mov(rsi, address_of(s.data))
;               [94:14] rsi = address_of(s.data)
;               [94:14] = expression
;               [94:14] address_of(s.data)
                lea rsi, [rbp + 1041]
;           [94:37] # buffer address
;           [95:5] mov(rdx, array_size_of(s.data))
;               [95:14] rdx = array_size_of(s.data)
;               [95:14] = expression
;               [95:14] array_size_of(s.data)
                mov rdx, 127
;           [95:37] # buffer size
;           [96:5] syscall()
            push r11
            syscall
            pop r11
;           [97:5] mov(s.len, rax - 1)
;               [97:16] instructions without scratch register 2, with 3
;               [97:16] rax
                mov byte [rbp + 1040], al
;               [97:22] s.len - 1
                sub byte [rbp + 1040], 1
;           [97:25] # return value
        str_in_311_9_end:
        if_312_12:
;       [312:12] ? nm.len == 0
;       [312:12] ? nm.len == 0
        cmp_312_12:
        cmp byte [rbp + 1040], 0
        jne if_314_19
        if_312_12_code:
;           [313:13] break
            jmp loop_306_5_end
        jmp if_312_9_end
        if_314_19:
;       [314:19] ? nm.len <= 4
;       [314:19] ? nm.len <= 4
        cmp_314_19:
        cmp byte [rbp + 1040], 4
        jg if_else_312_9
        if_314_19_code:
;           [315:13] print(prompt2)
;           [46:6] print(str : i8[])
            print_315_13:
;               [315:13] alias str -> prompt2
;               [47:4] sys_print(array_size_of(str), address_of(str))
;               [47:14] allocate named register rdx
;               [47:14] rdx = array_size_of(str)
;               [47:14] = expression
;               [47:14] array_size_of(str)
                mov rdx, 20
;               [47:34] allocate named register rsi
;               [47:34] rsi = address_of(str)
;               [47:34] = expression
;               [47:34] address_of(str)
                lea rsi, [rbp + 33]
;               [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_47_4_315_13:
;                   [47:4] alias len -> rdx
;                   [47:4] alias ptr -> rsi
;                   [39:5] mov(rax, 1)
;                       [39:14] 1
                        mov rax, 1
;                   [39:19] # write system call
;                   [40:5] mov(rdi, 0)
;                       [40:14] 0
                        mov rdi, 0
;                   [40:19] # file descriptor for standard out
;                   [41:5] mov(rsi, ptr)
;                       [41:14] ptr
;                   [41:19] # buffer address
;                   [42:5] mov(rdx, len)
;                       [42:14] len
;                   [42:19] # buffer size
;                   [43:5] syscall()
                    push r11
                    syscall
                    pop r11
;                   [47:4] free named register rsi
;                   [47:4] free named register rdx
                sys_print_47_4_315_13_end:
            print_315_13_end:
;           [316:13] continue
            jmp loop_306_5
        jmp if_312_9_end
        if_else_312_9:
;           [318:13] print(prompt3)
;           [46:6] print(str : i8[])
            print_318_13:
;               [318:13] alias str -> prompt3
;               [47:4] sys_print(array_size_of(str), address_of(str))
;               [47:14] allocate named register rdx
;               [47:14] rdx = array_size_of(str)
;               [47:14] = expression
;               [47:14] array_size_of(str)
                mov rdx, 6
;               [47:34] allocate named register rsi
;               [47:34] rsi = address_of(str)
;               [47:34] = expression
;               [47:34] address_of(str)
                lea rsi, [rbp + 53]
;               [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_47_4_318_13:
;                   [47:4] alias len -> rdx
;                   [47:4] alias ptr -> rsi
;                   [39:5] mov(rax, 1)
;                       [39:14] 1
                        mov rax, 1
;                   [39:19] # write system call
;                   [40:5] mov(rdi, 0)
;                       [40:14] 0
                        mov rdi, 0
;                   [40:19] # file descriptor for standard out
;                   [41:5] mov(rsi, ptr)
;                       [41:14] ptr
;                   [41:19] # buffer address
;                   [42:5] mov(rdx, len)
;                       [42:14] len
;                   [42:19] # buffer size
;                   [43:5] syscall()
                    push r11
                    syscall
                    pop r11
;                   [47:4] free named register rsi
;                   [47:4] free named register rdx
                sys_print_47_4_318_13_end:
            print_318_13_end:
;           [319:13] str_out(nm)
;           [100:6] str_out(s : str)
            str_out_319_13:
;               [319:13] alias s -> nm
;               [101:5] mov(rax, 1)
;                   [101:14] 1
                    mov rax, 1
;               [101:34] # write system call
;               [102:5] mov(rdi, 0)
;                   [102:14] 0
                    mov rdi, 0
;               [102:34] # file descriptor for standard out
;               [103:5] mov(rsi, address_of(s.data))
;                   [103:14] rsi = address_of(s.data)
;                   [103:14] = expression
;                   [103:14] address_of(s.data)
                    lea rsi, [rbp + 1041]
;               [103:34] # buffer address
;               [104:5] mov(rdx, s.len)
;                   [104:14] s.len
                    movsx rdx, byte [rbp + 1040]
;               [104:34] # buffer size
;               [105:5] syscall()
                push r11
                syscall
                pop r11
            str_out_319_13_end:
;           [320:13] print(dot)
;           [46:6] print(str : i8[])
            print_320_13:
;               [320:13] alias str -> dot
;               [47:4] sys_print(array_size_of(str), address_of(str))
;               [47:14] allocate named register rdx
;               [47:14] rdx = array_size_of(str)
;               [47:14] = expression
;               [47:14] array_size_of(str)
                mov rdx, 1
;               [47:34] allocate named register rsi
;               [47:34] rsi = address_of(str)
;               [47:34] = expression
;               [47:34] address_of(str)
                lea rsi, [rbp + 59]
;               [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_47_4_320_13:
;                   [47:4] alias len -> rdx
;                   [47:4] alias ptr -> rsi
;                   [39:5] mov(rax, 1)
;                       [39:14] 1
                        mov rax, 1
;                   [39:19] # write system call
;                   [40:5] mov(rdi, 0)
;                       [40:14] 0
                        mov rdi, 0
;                   [40:19] # file descriptor for standard out
;                   [41:5] mov(rsi, ptr)
;                       [41:14] ptr
;                   [41:19] # buffer address
;                   [42:5] mov(rdx, len)
;                       [42:14] len
;                   [42:19] # buffer size
;                   [43:5] syscall()
                    push r11
                    syscall
                    pop r11
;                   [47:4] free named register rsi
;                   [47:4] free named register rdx
                sys_print_47_4_320_13_end:
            print_320_13_end:
;           [321:13] print(nl)
;           [46:6] print(str : i8[])
            print_321_13:
;               [321:13] alias str -> nl
;               [47:4] sys_print(array_size_of(str), address_of(str))
;               [47:14] allocate named register rdx
;               [47:14] rdx = array_size_of(str)
;               [47:14] = expression
;               [47:14] array_size_of(str)
                mov rdx, 1
;               [47:34] allocate named register rsi
;               [47:34] rsi = address_of(str)
;               [47:34] = expression
;               [47:34] address_of(str)
                lea rsi, [rbp + 60]
;               [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_47_4_321_13:
;                   [47:4] alias len -> rdx
;                   [47:4] alias ptr -> rsi
;                   [39:5] mov(rax, 1)
;                       [39:14] 1
                        mov rax, 1
;                   [39:19] # write system call
;                   [40:5] mov(rdi, 0)
;                       [40:14] 0
                        mov rdi, 0
;                   [40:19] # file descriptor for standard out
;                   [41:5] mov(rsi, ptr)
;                       [41:14] ptr
;                   [41:19] # buffer address
;                   [42:5] mov(rdx, len)
;                       [42:14] len
;                   [42:19] # buffer size
;                   [43:5] syscall()
                    push r11
                    syscall
                    pop r11
;                   [47:4] free named register rsi
;                   [47:4] free named register rdx
                sys_print_47_4_321_13_end:
            print_321_13_end:
        if_312_9_end:
    jmp loop_306_5
    loop_306_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; 
;[125:15] noinline print_num(num)
print_num:
;   [125:25] num: i64 (8 B @ [rbx])
;   [126:5] var buf : i8[20]
;   [126:9] buf: i8[20] (20 B @ [rbx + 8])
;   [126:9] zero 20 * 1 B = 20 B
;   [126:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [127:5] var n = num
;   [127:9] n: i64 (8 B @ [rbx + 28])
;   [127:9] n = num
;   [127:13] num
;   [127:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [127:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
;   [127:13] free scratch register r14
;   [127:13] free scratch register r15
;   [128:5] var is_negative : bool = false
;   [128:9] is_negative: bool (1 B @ [rbx + 36])
;   [128:9] is_negative = false
    mov byte [rbx + 36], 0
    if_130_8:
;   [130:8] ? n < 0
;   [130:8] ? n < 0
    cmp_130_8:
    cmp qword [rbx + 28], 0
    jge if_130_5_end
    if_130_8_code:
;       [131:9] is_negative = true
        mov byte [rbx + 36], 1
;       [132:9] n = -n
;       [132:14] -n
        neg qword [rbx + 28]
    if_130_5_end:
;   [135:5] var i = 20
;   [135:9] i: i64 (8 B @ [rbx + 37])
;   [135:9] i = 20
;   [135:13] 20
    mov qword [rbx + 37], 20
;   [136:5] label
    loop_136_5:
;       [137:9] i = i - 1
;       [137:13] instructions without scratch register 1, with 3
;       [137:13] i
;       [137:17] i - 1
        sub qword [rbx + 37], 1
;       [138:9] var ascii = 48 + (n % 10)
;       [138:13] ascii: i64 (8 B @ [rbx + 45])
;       [138:13] ascii = 48 + (n % 10)
;       [138:21] instructions without scratch register 8, with 9
;       [138:21] 48
        mov qword [rbx + 45], 48
;       [138:27] ascii + (n % 10)
;       [138:27] allocate scratch register -> r15
;       [138:27] n
        mov r15, qword [rbx + 28]
;       [138:31] r15 % 10
;       [138:31] div const
;       [138:31] allocate named register rax
        mov rax, r15
;       [138:31] allocate named register rdx
        cqo
;       [138:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [138:31] free scratch register r14
        mov r15, rdx
;       [138:31] free named register rdx
;       [138:31] free named register rax
        add qword [rbx + 45], r15
;       [138:27] free scratch register r15
;       [139:9] # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
;       [140:9] buf[i] = ascii
;       [140:9] allocate scratch register -> r15
;       [140:13] set array index
;       [140:13] i
        mov r15, qword [rbx + 37]
;       [140:13] bounds check
;       [140:13] allocate scratch register -> r14
;       [140:13] line number
        mov r14, 140
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [140:13] free scratch register r14
;       [140:18] ascii
;       [140:18] allocate scratch register -> r14
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
;       [140:18] free scratch register r14
;       [140:9] free scratch register r15
;       [141:9] n = n / 10
;       [141:13] instructions without scratch register 5, with 7
;       [141:13] n
;       [141:17] n / 10
;       [141:17] div const
;       [141:17] allocate named register rax
        mov rax, qword [rbx + 28]
;       [141:17] allocate named register rdx
        cqo
;       [141:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [141:17] free scratch register r15
        mov qword [rbx + 28], rax
;       [141:17] free named register rdx
;       [141:17] free named register rax
        if_142_12:
;       [142:12] ? n == 0
;       [142:12] ? n == 0
        cmp_142_12:
        cmp qword [rbx + 28], 0
        jne if_142_9_end
        if_142_12_code:
;           [142:19] break
            jmp loop_136_5_end
        if_142_9_end:
    jmp loop_136_5
    loop_136_5_end:
    if_145_8:
;   [145:8] ? is_negative
;   [145:8] ? is_negative
    cmp_145_8:
    cmp byte [rbx + 36], 0
    je if_145_5_end
    if_145_8_code:
;       [146:9] i = i - 1
;       [146:13] instructions without scratch register 1, with 3
;       [146:13] i
;       [146:17] i - 1
        sub qword [rbx + 37], 1
;       [147:9] buf[i] = 45
;       [147:9] allocate scratch register -> r15
;       [147:13] set array index
;       [147:13] i
        mov r15, qword [rbx + 37]
;       [147:13] bounds check
;       [147:13] allocate scratch register -> r14
;       [147:13] line number
        mov r14, 147
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [147:13] free scratch register r14
;       [147:18] 45
        mov byte [rbx + r15 + 8], 45
;       [147:9] free scratch register r15
    if_145_5_end:
;   [150:5] var write_pos = 0
;   [150:9] write_pos: i64 (8 B @ [rbx + 45])
;   [150:9] write_pos = 0
;   [150:21] 0
    mov qword [rbx + 45], 0
;   [151:5] label
    loop_151_5:
;       [152:9] buf[write_pos] = buf[i]
;       [152:9] allocate scratch register -> r15
;       [152:13] set array index
;       [152:13] write_pos
        mov r15, qword [rbx + 45]
;       [152:13] bounds check
;       [152:13] allocate scratch register -> r14
;       [152:13] line number
        mov r14, 152
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [152:13] free scratch register r14
;       [152:26] buf[i]
;       [152:26] allocate scratch register -> r14
;       [152:30] set array index
;       [152:30] i
        mov r14, qword [rbx + 37]
;       [152:30] bounds check
;       [152:30] allocate scratch register -> r13
;       [152:30] line number
        mov r13, 152
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [152:30] free scratch register r13
;       [152:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [152:26] free scratch register r13
;       [152:26] free scratch register r14
;       [152:9] free scratch register r15
;       [153:9] write_pos = write_pos + 1
;       [153:21] instructions without scratch register 1, with 3
;       [153:21] write_pos
;       [153:33] write_pos + 1
        add qword [rbx + 45], 1
;       [154:9] i = i + 1
;       [154:13] instructions without scratch register 1, with 3
;       [154:13] i
;       [154:17] i + 1
        add qword [rbx + 37], 1
        if_155_12:
;       [155:12] ? i == 20
;       [155:12] ? i == 20
        cmp_155_12:
        cmp qword [rbx + 37], 20
        jne if_155_9_end
        if_155_12_code:
;           [155:20] break
            jmp loop_151_5_end
        if_155_9_end:
    jmp loop_151_5
    loop_151_5_end:
;   [158:5] sys_print(write_pos, address_of(buf))
;   [158:15] allocate named register rdx
    mov rdx, qword [rbx + 45]
;   [158:26] allocate named register rsi
;   [158:26] rsi = address_of(buf)
;   [158:26] = expression
;   [158:26] address_of(buf)
    lea rsi, [rbx + 8]
;   [38:6] sys_print(len : reg_rdx, ptr : reg_rsi)
    sys_print_158_5:
;       [158:5] alias len -> rdx
;       [158:5] alias ptr -> rsi
;       [39:5] mov(rax, 1)
;           [39:14] 1
            mov rax, 1
;       [39:19] # write system call
;       [40:5] mov(rdi, 0)
;           [40:14] 0
            mov rdi, 0
;       [40:19] # file descriptor for standard out
;       [41:5] mov(rsi, ptr)
;           [41:14] ptr
;       [41:19] # buffer address
;       [42:5] mov(rdx, len)
;           [42:14] len
;       [42:19] # buffer size
;       [43:5] syscall()
        push r11
        syscall
        pop r11
;       [158:5] free named register rsi
;       [158:5] free named register rdx
    sys_print_158_5_end:
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
    mov rdi, strict qword num_buffer + 19
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
    mov rdx, strict qword num_buffer + 20
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
