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
  `address_of`, `equal`, `mov`, `syscall`, `exit`, `read`, `write`

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
C/C++ Header                    52           2846            865           9932
C++                              1             46              7            231
-------------------------------------------------------------------------------
SUM:                            53           2892            872          10163
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

func print(str : i8[]) {
    write(1, address_of(str), array_size_of(str))
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
    var nbytes = read(0, address_of(s.data), array_size_of(s.data))
    s.len = nbytes - 1
} 

func str_out(s : str) {
    write(1, address_of(s.data), s.len)
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
 
    write(1, address_of(buf), write_pos)
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
    cmp_141_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_141_12:
    assert_141_5:
        if_36_26_141_5:
        cmp_36_26_141_5:
        cmp r15b, 0
        jne if_36_23_141_5_end
        if_36_26_141_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_141_5_end:
    assert_141_5_end:
    mov qword [rbp + 240], -1
    cmp_145_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_145_12:
    assert_145_5:
        if_36_26_145_5:
        cmp_36_26_145_5:
        cmp r15b, 0
        jne if_36_23_145_5_end
        if_36_26_145_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_145_5_end:
    assert_145_5_end:
        cmp_149_16:
        bool_end_149_16:
        mov r15b, 1
        assert_149_9:
            if_36_26_149_9:
            cmp_36_26_149_9:
            cmp r15b, 0
            jne if_36_23_149_9_end
            if_36_26_149_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_36_23_149_9_end:
        assert_149_9_end:
    cmp_152_12:
    bool_end_152_12:
    mov r15b, 1
    assert_152_5:
        if_36_26_152_5:
        cmp_36_26_152_5:
        cmp r15b, 0
        jne if_36_23_152_5_end
        if_36_26_152_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_152_5_end:
    assert_152_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 157
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 158
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_159_12:
    mov r14, 1
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_159_12:
    assert_159_5:
        if_36_26_159_5:
        cmp_36_26_159_5:
        cmp r15b, 0
        jne if_36_23_159_5_end
        if_36_26_159_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_159_5_end:
    assert_159_5_end:
    cmp_160_12:
    mov r14, 2
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_160_12:
    assert_160_5:
        if_36_26_160_5:
        cmp_36_26_160_5:
        cmp r15b, 0
        jne if_36_23_160_5_end
        if_36_26_160_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_160_5_end:
    assert_160_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 162
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_164_12:
    mov r14, 0
    mov r13, 164
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_36_26_164_5:
        cmp_36_26_164_5:
        cmp r15b, 0
        jne if_36_23_164_5_end
        if_36_26_164_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_164_5_end:
    assert_164_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_168_12:
        mov rcx, 4
        mov r13, 168
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 168
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
    bool_end_168_12:
    assert_168_5:
        if_36_26_168_5:
        cmp_36_26_168_5:
        cmp r15b, 0
        jne if_36_23_168_5_end
        if_36_26_168_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_168_5_end:
    assert_168_5_end:
    mov r15, 2
    mov r14, 171
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_172_12:
        mov rcx, 4
        mov r13, 172
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 172
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
    bool_end_172_12:
    assert_172_5:
        if_36_26_172_5:
        cmp_36_26_172_5:
        cmp r15b, 0
        jne if_36_23_172_5_end
        if_36_26_172_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_172_5_end:
    assert_172_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 175
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_175_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_175_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_176_12:
    mov r14, qword [rbp + 248]
    mov r13, 176
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_36_26_176_5:
        cmp_36_26_176_5:
        cmp r15b, 0
        jne if_36_23_176_5_end
        if_36_26_176_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_176_5_end:
    assert_176_5_end:
    faz_178_5:
        mov r15, 1
        mov r14, 71
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_178_5_end:
    cmp_179_12:
    mov r14, 1
    mov r13, 179
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_179_12:
    assert_179_5:
        if_36_26_179_5:
        cmp_36_26_179_5:
        cmp r15b, 0
        jne if_36_23_179_5_end
        if_36_26_179_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_179_5_end:
    assert_179_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_182_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_182_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_182_5
    foo_182_5_end:
    cmp_185_12:
    mov r14, 0
    mov r13, 185
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
    bool_end_185_12:
    assert_185_5:
        if_36_26_185_5:
        cmp_36_26_185_5:
        cmp r15b, 0
        jne if_36_23_185_5_end
        if_36_26_185_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_185_5_end:
    assert_185_5_end:
    cmp_186_12:
    mov r14, 1
    mov r13, 186
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
    bool_end_186_12:
    assert_186_5:
        if_36_26_186_5:
        cmp_36_26_186_5:
        cmp r15b, 0
        jne if_36_23_186_5_end
        if_36_26_186_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_186_5_end:
    assert_186_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_193_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_193_5_end:
    cmp_194_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_194_12:
    assert_194_5:
        if_36_26_194_5:
        cmp_36_26_194_5:
        cmp r15b, 0
        jne if_36_23_194_5_end
        if_36_26_194_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_194_5_end:
    assert_194_5_end:
    cmp_195_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_36_26_195_5:
        cmp_36_26_195_5:
        cmp r15b, 0
        jne if_36_23_195_5_end
        if_36_26_195_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_195_5_end:
    assert_195_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_198_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_198_12:
    assert_198_5:
        if_36_26_198_5:
        cmp_36_26_198_5:
        cmp r15b, 0
        jne if_36_23_198_5_end
        if_36_26_198_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_198_5_end:
    assert_198_5_end:
    mov qword [rbp + 320], 3
    cmp_203_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_36_26_203_5:
        cmp_36_26_203_5:
        cmp r15b, 0
        jne if_36_23_203_5_end
        if_36_26_203_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 336], 0
    bar_206_5:
        if_52_8_206_5:
        cmp_52_8_206_5:
        cmp qword [rbp + 336], 0
        jne if_52_5_206_5_end
        if_52_8_206_5_code:
            jmp bar_206_5_end
        if_52_5_206_5_end:
        mov qword [rbp + 336], 255
    bar_206_5_end:
    cmp_207_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_36_26_207_5:
        cmp_36_26_207_5:
        cmp r15b, 0
        jne if_36_23_207_5_end
        if_36_26_207_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_207_5_end:
    assert_207_5_end:
    mov qword [rbp + 336], 1
    bar_210_5:
        if_52_8_210_5:
        cmp_52_8_210_5:
        cmp qword [rbp + 336], 0
        jne if_52_5_210_5_end
        if_52_8_210_5_code:
            jmp bar_210_5_end
        if_52_5_210_5_end:
        mov qword [rbp + 336], 255
    bar_210_5_end:
    cmp_211_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_36_26_211_5:
        cmp_36_26_211_5:
        cmp r15b, 0
        jne if_36_23_211_5_end
        if_36_26_211_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_211_5_end:
    assert_211_5_end:
    mov qword [rbp + 344], 1
    baz_214_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_214_13_end:
    cmp_215_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_36_26_215_5:
        cmp_36_26_215_5:
        cmp r15b, 0
        jne if_36_23_215_5_end
        if_36_26_215_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_215_5_end:
    assert_215_5_end:
    baz_217_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_217_9_end:
    cmp_218_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_36_26_218_5:
        cmp_36_26_218_5:
        cmp r15b, 0
        jne if_36_23_218_5_end
        if_36_26_218_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_218_5_end:
    assert_218_5_end:
    baz_220_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_220_23_end:
    mov qword [rbp + 368], 0
    cmp_221_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_36_26_221_5:
        cmp_36_26_221_5:
        cmp r15b, 0
        jne if_36_23_221_5_end
        if_36_26_221_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_221_5_end:
    assert_221_5_end:
    point_init_223_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_223_22_end:
    cmp_224_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_224_12:
    assert_224_5:
        if_36_26_224_5:
        cmp_36_26_224_5:
        cmp r15b, 0
        jne if_36_23_224_5_end
        if_36_26_224_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_224_5_end:
    assert_224_5_end:
    cmp_225_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_36_26_225_5:
        cmp_36_26_225_5:
        cmp r15b, 0
        jne if_36_23_225_5_end
        if_36_26_225_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_225_5_end:
    assert_225_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_231_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_36_26_231_5:
        cmp_36_26_231_5:
        cmp r15b, 0
        jne if_36_23_231_5_end
        if_36_26_231_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_231_5_end:
    assert_231_5_end:
    cmp_232_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_36_26_232_5:
        cmp_36_26_232_5:
        cmp r15b, 0
        jne if_36_23_232_5_end
        if_36_26_232_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_232_5_end:
    assert_232_5_end:
    cmp_233_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_36_26_233_5:
        cmp_36_26_233_5:
        cmp r15b, 0
        jne if_36_23_233_5_end
        if_36_26_233_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_233_5_end:
    assert_233_5_end:
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
    cmp_237_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_36_26_237_5:
        cmp_36_26_237_5:
        cmp r15b, 0
        jne if_36_23_237_5_end
        if_36_26_237_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_237_5_end:
    assert_237_5_end:
    cmp_238_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_36_26_238_5:
        cmp_36_26_238_5:
        cmp r15b, 0
        jne if_36_23_238_5_end
        if_36_26_238_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_238_5_end:
    assert_238_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_241_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_241_12:
    assert_241_5:
        if_36_26_241_5:
        cmp_36_26_241_5:
        cmp r15b, 0
        jne if_36_23_241_5_end
        if_36_26_241_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_241_5_end:
    assert_241_5_end:
    cmp_242_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_242_12:
    assert_242_5:
        if_36_26_242_5:
        cmp_36_26_242_5:
        cmp r15b, 0
        jne if_36_23_242_5_end
        if_36_26_242_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_242_5_end:
    assert_242_5_end:
    cmp_243_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_243_12:
    assert_243_5:
        if_36_26_243_5:
        cmp_36_26_243_5:
        cmp r15b, 0
        jne if_36_23_243_5_end
        if_36_26_243_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_243_5_end:
    assert_243_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_249_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 249
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
    bool_end_249_12:
    assert_249_5:
        if_36_26_249_5:
        cmp_36_26_249_5:
        cmp r15b, 0
        jne if_36_23_249_5_end
        if_36_26_249_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_249_5_end:
    assert_249_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_251_13:
        mov qword [r15 + 8], 74
    object_init_251_13_end:
    cmp_252_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 252
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
    bool_end_252_12:
    assert_252_5:
        if_36_26_252_5:
        cmp_36_26_252_5:
        cmp r15b, 0
        jne if_36_23_252_5_end
        if_36_26_252_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_252_5_end:
    assert_252_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_256_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_36_26_256_5:
        cmp_36_26_256_5:
        cmp r15b, 0
        jne if_36_23_256_5_end
        if_36_26_256_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_256_5_end:
    assert_256_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 259
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 259
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 260
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_266_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_266_12:
    assert_266_5:
        if_36_26_266_5:
        cmp_36_26_266_5:
        cmp r15b, 0
        jne if_36_23_266_5_end
        if_36_26_266_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_266_5_end:
    assert_266_5_end:
    cmp_267_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r10, 268
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 268
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r10, 269
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 269
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
    bool_end_267_12:
    assert_267_5:
        if_36_26_267_5:
        cmp_36_26_267_5:
        cmp r15b, 0
        jne if_36_23_267_5_end
        if_36_26_267_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_267_5_end:
    assert_267_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_274_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_36_26_274_5:
        cmp_36_26_274_5:
        cmp r15b, 0
        jne if_36_23_274_5_end
        if_36_26_274_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_274_5_end:
    assert_274_5_end:
    cmp_275_12:
    mov r14, 0
    mov r13, 275
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_275_12:
    assert_275_5:
        if_36_26_275_5:
        cmp_36_26_275_5:
        cmp r15b, 0
        jne if_36_23_275_5_end
        if_36_26_275_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_275_5_end:
    assert_275_5_end:
    cmp_276_12:
    mov r14, 1
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_276_12:
    assert_276_5:
        if_36_26_276_5:
        cmp_36_26_276_5:
        cmp r15b, 0
        jne if_36_23_276_5_end
        if_36_26_276_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_276_5_end:
    assert_276_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_280_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_280_5_end:
    loop_281_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_284_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_284_9_end:
        print_285_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_285_9_end:
        str_in_286_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_286_9_end:
        if_287_12:
        cmp_287_12:
        cmp byte [rbp + 1040], 0
        jne if_289_19
        if_287_12_code:
            jmp loop_281_5_end
        jmp if_287_9_end
        if_289_19:
        cmp_289_19:
        cmp byte [rbp + 1040], 4
        jg if_else_287_9
        if_289_19_code:
            print_290_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_290_13_end:
            jmp loop_281_5
        jmp if_287_9_end
        if_else_287_9:
            print_293_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_293_13_end:
            str_out_294_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_294_13_end:
            print_295_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_295_13_end:
            print_296_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_296_13_end:
        if_287_9_end:
    jmp loop_281_5
    loop_281_5_end:
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
    if_105_8:
    cmp_105_8:
    cmp qword [rbx + 28], 0
    jge if_105_5_end
    if_105_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_105_5_end:
    mov qword [rbx + 37], 20
    loop_111_5:
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
        mov r14, 115
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
        if_117_12:
        cmp_117_12:
        cmp qword [rbx + 28], 0
        jne if_117_9_end
        if_117_12_code:
            jmp loop_111_5_end
        if_117_9_end:
    jmp loop_111_5
    loop_111_5_end:
    if_120_8:
    cmp_120_8:
    cmp byte [rbx + 36], 0
    je if_120_5_end
    if_120_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 122
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_120_5_end:
    mov qword [rbx + 45], 0
    loop_126_5:
        mov r15, qword [rbx + 45]
        mov r14, 127
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 127
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
        if_130_12:
        cmp_130_12:
        cmp qword [rbx + 37], 20
        jne if_130_9_end
        if_130_12_code:
            jmp loop_126_5_end
        if_130_9_end:
    jmp loop_126_5
    loop_126_5_end:
    mov rdi, 1
    lea rsi, [rbx + 8]
    mov rdx, qword [rbx + 45]
    mov rax, 1
    syscall
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
;[42:1] # function arguments and return are equivalent to mutable references
;[49:1] # default argument type is `i64`
;[56:1] # return target is specified as a variable, in this case `res`
;[58:1] # return variable is a mutable reference to destination
;[68:1] # array arguments are declared with type and []
;[92:7] const yes = 1
;[93:7] const no = 0
;[94:7] const maybe = -1
;[96:1] # constants can be declared in any scope and shadow outer declarations
;[98:1] # limited support for non-inlined functions
; 
main:
;   [137:5] var arr : i32[4]
;   [137:9] arr: i32[4] (16 B @ [rbp + 224])
;   [137:9] zero 4 * 4 B = 16 B
;   [137:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [138:5] # arrays are initialized to 0
;   [140:5] var answer
;   [140:9] answer: i64 (8 B @ [rbp + 240])
;   [140:9] zero 1 * 8 B = 8 B
;   [140:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [141:5] assert(answer == 0)
;   [141:12] allocate scratch register -> r15
;   [141:12] ? answer == 0
;   [141:12] ? answer == 0
    cmp_141_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_141_12:
;   [36:6] assert(x : bool)
    assert_141_5:
;       [141:5] alias x -> r15b
        if_36_26_141_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_141_5:
        cmp r15b, 0
        jne if_36_23_141_5_end
        if_36_26_141_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_141_5_end:
;       [141:5] free scratch register r15
    assert_141_5_end:
;   [142:5] # variables without initializer are zeroed
;   [144:5] answer = maybe
;   [144:14] maybe
    mov qword [rbp + 240], -1
;   [145:5] assert(answer == -1)
;   [145:12] allocate scratch register -> r15
;   [145:12] ? answer == -1
;   [145:12] ? answer == -1
    cmp_145_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_145_12:
;   [36:6] assert(x : bool)
    assert_145_5:
;       [145:5] alias x -> r15b
        if_36_26_145_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_145_5:
        cmp r15b, 0
        jne if_36_23_145_5_end
        if_36_26_145_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_145_5_end:
;       [145:5] free scratch register r15
    assert_145_5_end:
;       [148:15] const maybe = 33
;       [149:9] assert(maybe == 33)
;       [149:16] allocate scratch register -> r15
;       [149:16] ? maybe == 33
;       [149:16] ? maybe == 33
        cmp_149_16:
;       [149:16] const eval to true
        bool_end_149_16:
        mov r15b, 1
;       [36:6] assert(x : bool)
        assert_149_9:
;           [149:9] alias x -> r15b
            if_36_26_149_9:
;           [36:26] ? not x
;           [36:26] ? not x
            cmp_36_26_149_9:
            cmp r15b, 0
            jne if_36_23_149_9_end
            if_36_26_149_9_code:
;               [36:32] exit(1)
;               [36:32] allocate named register rdi
;               [36:37] 1
                mov rdi, 1
                mov rax, 60
                syscall
;               [36:32] free named register rdi
            if_36_23_149_9_end:
;           [149:9] free scratch register r15
        assert_149_9_end:
;   [152:5] assert(maybe == -1)
;   [152:12] allocate scratch register -> r15
;   [152:12] ? maybe == -1
;   [152:12] ? maybe == -1
    cmp_152_12:
;   [152:12] const eval to true
    bool_end_152_12:
    mov r15b, 1
;   [36:6] assert(x : bool)
    assert_152_5:
;       [152:5] alias x -> r15b
        if_36_26_152_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_152_5:
        cmp r15b, 0
        jne if_36_23_152_5_end
        if_36_26_152_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_152_5_end:
;       [152:5] free scratch register r15
    assert_152_5_end:
;   [154:5] var ix = 1
;   [154:9] ix: i64 (8 B @ [rbp + 248])
;   [154:9] ix = 1
;   [154:14] 1
    mov qword [rbp + 248], 1
;   [155:5] # variables can have an initial value that can be an expression
;   [157:5] arr[ix] = 2
;   [157:5] allocate scratch register -> r15
;   [157:9] set array index
;   [157:9] ix
    mov r15, qword [rbp + 248]
;   [157:9] bounds check
;   [157:9] allocate scratch register -> r14
;   [157:9] line number
    mov r14, 157
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [157:9] free scratch register r14
;   [157:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [157:5] free scratch register r15
;   [158:5] arr[ix + 1] = arr[ix]
;   [158:5] allocate scratch register -> r15
;   [158:9] set array index
;   [158:9] ix
    mov r15, qword [rbp + 248]
;   [158:14] r15 + 1
    add r15, 1
;   [158:9] bounds check
;   [158:9] allocate scratch register -> r14
;   [158:9] line number
    mov r14, 158
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [158:9] free scratch register r14
;   [158:19] arr[ix]
;   [158:19] allocate scratch register -> r14
;   [158:23] set array index
;   [158:23] ix
    mov r14, qword [rbp + 248]
;   [158:23] bounds check
;   [158:23] allocate scratch register -> r13
;   [158:23] line number
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [158:23] free scratch register r13
;   [158:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [158:19] free scratch register r13
;   [158:19] free scratch register r14
;   [158:5] free scratch register r15
;   [159:5] assert(arr[1] == 2)
;   [159:12] allocate scratch register -> r15
;   [159:12] ? arr[1] == 2
;   [159:12] ? arr[1] == 2
    cmp_159_12:
;   [159:12] allocate scratch register -> r14
;   [159:16] set array index
;   [159:16] 1
    mov r14, 1
;   [159:16] bounds check
;   [159:16] allocate scratch register -> r13
;   [159:16] line number
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [159:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [159:12] free scratch register r14
    sete r15b
    bool_end_159_12:
;   [36:6] assert(x : bool)
    assert_159_5:
;       [159:5] alias x -> r15b
        if_36_26_159_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_159_5:
        cmp r15b, 0
        jne if_36_23_159_5_end
        if_36_26_159_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_159_5_end:
;       [159:5] free scratch register r15
    assert_159_5_end:
;   [160:5] assert(arr[2] == 2)
;   [160:12] allocate scratch register -> r15
;   [160:12] ? arr[2] == 2
;   [160:12] ? arr[2] == 2
    cmp_160_12:
;   [160:12] allocate scratch register -> r14
;   [160:16] set array index
;   [160:16] 2
    mov r14, 2
;   [160:16] bounds check
;   [160:16] allocate scratch register -> r13
;   [160:16] line number
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [160:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [160:12] free scratch register r14
    sete r15b
    bool_end_160_12:
;   [36:6] assert(x : bool)
    assert_160_5:
;       [160:5] alias x -> r15b
        if_36_26_160_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_160_5:
        cmp r15b, 0
        jne if_36_23_160_5_end
        if_36_26_160_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_160_5_end:
;       [160:5] free scratch register r15
    assert_160_5_end:
;   [162:5] array_copy(arr[2], arr, 2)
;   [162:5] allocate named register rsi
;   [162:5] allocate named register rdi
;   [162:5] allocate named register rcx
;   [162:29] 2
;   [162:29] 2
    mov rcx, 2
;   [162:16] arr[2]
;   [162:16] allocate scratch register -> r15
;   [162:20] set array index
;   [162:20] 2
    mov r15, 2
;   [162:20] bounds check
;   [162:20] allocate scratch register -> r14
;   [162:20] line number
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [162:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [162:20] free scratch register r13
    cmovg rbp, r14
    jg baz_bounds_panic
;   [162:20] free scratch register r14
    lea rsi, [rbp + r15 * 4 + 224]
;   [162:5] free scratch register r15
;   [162:24] arr
;   [162:24] bounds check
;   [162:24] allocate scratch register -> r15
;   [162:24] line number
    mov r15, 162
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [162:24] free scratch register r15
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
;   [162:5] free named register rcx
;   [162:5] free named register rdi
;   [162:5] free named register rsi
;   [163:5] # copy from, to, number of elements
;   [164:5] assert(arr[0] == 2)
;   [164:12] allocate scratch register -> r15
;   [164:12] ? arr[0] == 2
;   [164:12] ? arr[0] == 2
    cmp_164_12:
;   [164:12] allocate scratch register -> r14
;   [164:16] set array index
;   [164:16] 0
    mov r14, 0
;   [164:16] bounds check
;   [164:16] allocate scratch register -> r13
;   [164:16] line number
    mov r13, 164
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [164:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [164:12] free scratch register r14
    sete r15b
    bool_end_164_12:
;   [36:6] assert(x : bool)
    assert_164_5:
;       [164:5] alias x -> r15b
        if_36_26_164_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_164_5:
        cmp r15b, 0
        jne if_36_23_164_5_end
        if_36_26_164_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_164_5_end:
;       [164:5] free scratch register r15
    assert_164_5_end:
;   [166:5] var arr1 : i32[8]
;   [166:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [166:9] zero 8 * 4 B = 32 B
;   [166:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [167:5] array_copy(arr, arr1, 4)
;   [167:5] allocate named register rsi
;   [167:5] allocate named register rdi
;   [167:5] allocate named register rcx
;   [167:27] 4
;   [167:27] 4
    mov rcx, 4
;   [167:16] arr
;   [167:16] bounds check
;   [167:16] allocate scratch register -> r15
;   [167:16] line number
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [167:16] free scratch register r15
    lea rsi, [rbp + 224]
;   [167:21] arr1
;   [167:21] bounds check
;   [167:21] allocate scratch register -> r15
;   [167:21] line number
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [167:21] free scratch register r15
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
;   [167:5] free named register rcx
;   [167:5] free named register rdi
;   [167:5] free named register rsi
;   [168:5] assert(arrays_equal(arr, arr1, 4))
;   [168:12] allocate scratch register -> r15
;   [168:12] ? arrays_equal(arr, arr1, 4)
;   [168:12] ? arrays_equal(arr, arr1, 4)
    cmp_168_12:
;   [168:12] allocate scratch register -> r14
;       [168:12] r14b = arrays_equal(arr, arr1, 4)
;       [168:12] = expression
;       [168:12] arrays_equal(arr, arr1, 4)
;       [168:12] allocate named register rsi
;       [168:12] allocate named register rdi
;       [168:12] allocate named register rcx
;       [168:36] 4
;       [168:36] 4
        mov rcx, 4
;       [168:25] arr
;       [168:25] bounds check
;       [168:25] allocate scratch register -> r13
;       [168:25] line number
        mov r13, 168
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [168:25] free scratch register r13
        lea rsi, [rbp + 224]
;       [168:30] arr1
;       [168:30] bounds check
;       [168:30] allocate scratch register -> r13
;       [168:30] line number
        mov r13, 168
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [168:30] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [168:12] free named register rcx
;       [168:12] free named register rdi
;       [168:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [168:12] free scratch register r14
    setne r15b
    bool_end_168_12:
;   [36:6] assert(x : bool)
    assert_168_5:
;       [168:5] alias x -> r15b
        if_36_26_168_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_168_5:
        cmp r15b, 0
        jne if_36_23_168_5_end
        if_36_26_168_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_168_5_end:
;       [168:5] free scratch register r15
    assert_168_5_end:
;   [169:5] # `arrays_equal` is built-in function
;   [171:5] arr1[2] = -1
;   [171:5] allocate scratch register -> r15
;   [171:10] set array index
;   [171:10] 2
    mov r15, 2
;   [171:10] bounds check
;   [171:10] allocate scratch register -> r14
;   [171:10] line number
    mov r14, 171
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [171:10] free scratch register r14
;   [171:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [171:5] free scratch register r15
;   [172:5] assert(not arrays_equal(arr, arr1, 4))
;   [172:12] allocate scratch register -> r15
;   [172:12] ? not arrays_equal(arr, arr1, 4)
;   [172:12] ? not arrays_equal(arr, arr1, 4)
    cmp_172_12:
;   [172:16] allocate scratch register -> r14
;       [172:16] r14b = arrays_equal(arr, arr1, 4)
;       [172:16] = expression
;       [172:16] arrays_equal(arr, arr1, 4)
;       [172:16] allocate named register rsi
;       [172:16] allocate named register rdi
;       [172:16] allocate named register rcx
;       [172:40] 4
;       [172:40] 4
        mov rcx, 4
;       [172:29] arr
;       [172:29] bounds check
;       [172:29] allocate scratch register -> r13
;       [172:29] line number
        mov r13, 172
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [172:29] free scratch register r13
        lea rsi, [rbp + 224]
;       [172:34] arr1
;       [172:34] bounds check
;       [172:34] allocate scratch register -> r13
;       [172:34] line number
        mov r13, 172
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [172:34] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [172:16] free named register rcx
;       [172:16] free named register rdi
;       [172:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [172:12] free scratch register r14
    sete r15b
    bool_end_172_12:
;   [36:6] assert(x : bool)
    assert_172_5:
;       [172:5] alias x -> r15b
        if_36_26_172_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_172_5:
        cmp r15b, 0
        jne if_36_23_172_5_end
        if_36_26_172_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_172_5_end:
;       [172:5] free scratch register r15
    assert_172_5_end:
;   [174:5] ix = 3
;   [174:10] 3
    mov qword [rbp + 248], 3
;   [175:5] arr[ix] = ~inv(arr[ix - 1])
;   [175:5] allocate scratch register -> r15
;   [175:9] set array index
;   [175:9] ix
    mov r15, qword [rbp + 248]
;   [175:9] bounds check
;   [175:9] allocate scratch register -> r14
;   [175:9] line number
    mov r14, 175
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [175:9] free scratch register r14
;   [175:16] arr = ~inv(arr[ix - 1])
;   [175:16] = expression
;   [175:16] ~inv(arr[ix - 1])
;   [175:20] allocate scratch register -> r14
;   [175:24] set array index
;   [175:24] ix
    mov r14, qword [rbp + 248]
;   [175:29] r14 - 1
    sub r14, 1
;   [175:24] bounds check
;   [175:24] allocate scratch register -> r13
;   [175:24] line number
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [175:24] free scratch register r13
;   [60:6] inv(i : i32) : i32 res
    inv_175_16:
;       [175:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [175:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [61:5] res = ~i
;       [61:12] ~i
;       [61:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [61:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [175:16] free scratch register r14
    inv_175_16_end:
    not dword [rbp + r15 * 4 + 224]
;   [175:5] free scratch register r15
;   [176:5] assert(arr[ix] == 2)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? arr[ix] == 2
;   [176:12] ? arr[ix] == 2
    cmp_176_12:
;   [176:12] allocate scratch register -> r14
;   [176:16] set array index
;   [176:16] ix
    mov r14, qword [rbp + 248]
;   [176:16] bounds check
;   [176:16] allocate scratch register -> r13
;   [176:16] line number
    mov r13, 176
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [176:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [176:12] free scratch register r14
    sete r15b
    bool_end_176_12:
;   [36:6] assert(x : bool)
    assert_176_5:
;       [176:5] alias x -> r15b
        if_36_26_176_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_176_5:
        cmp r15b, 0
        jne if_36_23_176_5_end
        if_36_26_176_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_176_5_end:
;       [176:5] free scratch register r15
    assert_176_5_end:
;   [178:5] faz(arr)
;   [70:6] faz(arg : i32[])
    faz_178_5:
;       [178:5] alias arg -> arr
;       [71:5] arg[1] = 0xfe
;       [71:5] allocate scratch register -> r15
;       [71:9] set array index
;       [71:9] 1
        mov r15, 1
;       [71:9] bounds check
;       [71:9] allocate scratch register -> r14
;       [71:9] line number
        mov r14, 71
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
;       [71:9] free scratch register r14
;       [71:14] 0xfe
        mov dword [rbp + r15 * 4 + 224], 254
;       [71:5] free scratch register r15
    faz_178_5_end:
;   [179:5] assert(arr[1] == 0xfe)
;   [179:12] allocate scratch register -> r15
;   [179:12] ? arr[1] == 0xfe
;   [179:12] ? arr[1] == 0xfe
    cmp_179_12:
;   [179:12] allocate scratch register -> r14
;   [179:16] set array index
;   [179:16] 1
    mov r14, 1
;   [179:16] bounds check
;   [179:16] allocate scratch register -> r13
;   [179:16] line number
    mov r13, 179
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [179:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 254
;   [179:12] free scratch register r14
    sete r15b
    bool_end_179_12:
;   [36:6] assert(x : bool)
    assert_179_5:
;       [179:5] alias x -> r15b
        if_36_26_179_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_179_5:
        cmp r15b, 0
        jne if_36_23_179_5_end
        if_36_26_179_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_179_5_end:
;       [179:5] free scratch register r15
    assert_179_5_end:
;   [181:5] var arr3 : i64[] = { 3, 5 }
;   [181:9] arr3: i64[2] (16 B @ [rbp + 288])
;   [181:9] arr3 = { 3, 5 }
;   [181:26] [0]
;   [181:26] 3
    mov qword [rbp + 288], 3
;   [181:26] [1]
;   [181:29] 5
    mov qword [rbp + 296], 5
;   [182:5] foo arr3
;   [182:9] allocate scratch register -> r15
;   [182:9] e: i64 (r15)
;   [182:9] i: i64 (8 B @ [rbp + 312])
;   [182:9] const n = 2
;   [182:9] initiate iterator e
    lea r15, [rbp + 288]
;   [182:9] initiate counter i
    mov qword [rbp + 312], 0
    foo_182_5:
;       [183:9] e = e + i + n
;       [183:13] instructions without scratch register 3, with 4
;       [183:13] e
;       [183:17] e + i
;       [183:17] allocate scratch register -> r14
        mov r14, qword [rbp + 312]
        add qword [r15], r14
;       [183:17] free scratch register r14
;       [183:21] e + n
        add qword [r15], 2
        foo_182_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_182_5
    foo_182_5_end:
;   [182:5] free scratch register r15
;   [185:5] assert(arr3[0] == 3 + 0 + 2)
;   [185:12] allocate scratch register -> r15
;   [185:12] ? arr3[0] == 3 + 0 + 2
;   [185:12] ? arr3[0] == 3 + 0 + 2
    cmp_185_12:
;   [185:12] allocate scratch register -> r14
;   [185:17] set array index
;   [185:17] 0
    mov r14, 0
;   [185:17] bounds check
;   [185:17] allocate scratch register -> r13
;   [185:17] line number
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [185:17] free scratch register r13
;   [185:23] allocate scratch register -> r13
;       [185:23] 3
        mov r13, 3
;       [185:27] r13 + 0
        add r13, 0
;       [185:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [185:12] free scratch register r13
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_185_5_end:
;       [185:5] free scratch register r15
    assert_185_5_end:
;   [186:5] assert(arr3[1] == 5 + 1 + 2)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? arr3[1] == 5 + 1 + 2
;   [186:12] ? arr3[1] == 5 + 1 + 2
    cmp_186_12:
;   [186:12] allocate scratch register -> r14
;   [186:17] set array index
;   [186:17] 1
    mov r14, 1
;   [186:17] bounds check
;   [186:17] allocate scratch register -> r13
;   [186:17] line number
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [186:17] free scratch register r13
;   [186:23] allocate scratch register -> r13
;       [186:23] 5
        mov r13, 5
;       [186:27] r13 + 1
        add r13, 1
;       [186:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [186:12] free scratch register r13
;   [186:12] free scratch register r14
    sete r15b
    bool_end_186_12:
;   [36:6] assert(x : bool)
    assert_186_5:
;       [186:5] alias x -> r15b
        if_36_26_186_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_186_5:
        cmp r15b, 0
        jne if_36_23_186_5_end
        if_36_26_186_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_186_5_end:
;       [186:5] free scratch register r15
    assert_186_5_end:
;   [187:5] # `foo` is a language construct that iterates over an array injecting:
;   [188:5] # `e`: current element
;   [189:5] # `i`: index starting at 0
;   [190:5] # `n`: constant array size
;   [192:5] var p : point = {0, 0}
;   [192:9] p: point (16 B @ [rbp + 304])
;   [192:9] p = {0, 0}
;   [192:22] copy field 'x'
    mov qword [rbp + 304], 0
;   [192:25] copy field 'y'
    mov qword [rbp + 312], 0
;   [193:5] fooz(p)
;   [44:6] fooz(pt : point)
    fooz_193_5:
;       [193:5] alias pt -> p
;       [45:5] pt.x = 0b10
;       [45:12] 0b10
        mov qword [rbp + 304], 2
;       [45:20] # binary value 2
;       [46:5] pt.y = 0xb
;       [46:12] 0xb
        mov qword [rbp + 312], 11
;       [46:20] # hex value 11
    fooz_193_5_end:
;   [194:5] assert(p.x == 2)
;   [194:12] allocate scratch register -> r15
;   [194:12] ? p.x == 2
;   [194:12] ? p.x == 2
    cmp_194_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_194_12:
;   [36:6] assert(x : bool)
    assert_194_5:
;       [194:5] alias x -> r15b
        if_36_26_194_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_194_5:
        cmp r15b, 0
        jne if_36_23_194_5_end
        if_36_26_194_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_194_5_end:
;       [194:5] free scratch register r15
    assert_194_5_end:
;   [195:5] assert(p.y == 0xb)
;   [195:12] allocate scratch register -> r15
;   [195:12] ? p.y == 0xb
;   [195:12] ? p.y == 0xb
    cmp_195_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_195_12:
;   [36:6] assert(x : bool)
    assert_195_5:
;       [195:5] alias x -> r15b
        if_36_26_195_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_195_5:
        cmp r15b, 0
        jne if_36_23_195_5_end
        if_36_26_195_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_195_5_end:
;       [195:5] free scratch register r15
    assert_195_5_end:
;   [197:5] var q : point = p
;   [197:9] q: point (16 B @ [rbp + 320])
;   [197:9] q = p
;   [197:21] size <= 16 B, use mov
;   [197:21] allocate named register rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
;   [197:21] free named register rax
;   [198:5] assert(equal(p, q))
;   [198:12] allocate scratch register -> r15
;   [198:12] ? equal(p, q)
;   [198:12] ? equal(p, q)
    cmp_198_12:
;   [198:12] allocate scratch register -> r14
;       [198:12] r14b = equal(p, q)
;       [198:12] = expression
;       [198:12] equal(p, q)
;       [198:12] allocate named register rsi
;       [198:12] allocate named register rdi
;       [198:12] allocate named register rcx
;       [198:18] p
        lea rsi, [rbp + 304]
;       [198:21] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [198:12] free named register rcx
;       [198:12] free named register rdi
;       [198:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [198:12] free scratch register r14
    setne r15b
    bool_end_198_12:
;   [36:6] assert(x : bool)
    assert_198_5:
;       [198:5] alias x -> r15b
        if_36_26_198_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_198_5:
        cmp r15b, 0
        jne if_36_23_198_5_end
        if_36_26_198_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_198_5_end:
;       [198:5] free scratch register r15
    assert_198_5_end:
;   [199:5] # `equal` is built-in function to compare user types for equality or same
;   [200:5] # size arrays
;   [202:5] q.x = 3
;   [202:11] 3
    mov qword [rbp + 320], 3
;   [203:5] assert(not equal(p, q))
;   [203:12] allocate scratch register -> r15
;   [203:12] ? not equal(p, q)
;   [203:12] ? not equal(p, q)
    cmp_203_12:
;   [203:16] allocate scratch register -> r14
;       [203:16] r14b = equal(p, q)
;       [203:16] = expression
;       [203:16] equal(p, q)
;       [203:16] allocate named register rsi
;       [203:16] allocate named register rdi
;       [203:16] allocate named register rcx
;       [203:22] p
        lea rsi, [rbp + 304]
;       [203:25] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [203:16] free named register rcx
;       [203:16] free named register rdi
;       [203:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [203:12] free scratch register r14
    sete r15b
    bool_end_203_12:
;   [36:6] assert(x : bool)
    assert_203_5:
;       [203:5] alias x -> r15b
        if_36_26_203_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_203_5:
        cmp r15b, 0
        jne if_36_23_203_5_end
        if_36_26_203_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_203_5_end:
;       [203:5] free scratch register r15
    assert_203_5_end:
;   [205:5] var i = 0
;   [205:9] i: i64 (8 B @ [rbp + 336])
;   [205:9] i = 0
;   [205:13] 0
    mov qword [rbp + 336], 0
;   [206:5] bar(i)
;   [51:6] bar(arg)
    bar_206_5:
;       [206:5] alias arg -> i
        if_52_8_206_5:
;       [52:8] ? arg == 0
;       [52:8] ? arg == 0
        cmp_52_8_206_5:
        cmp qword [rbp + 336], 0
        jne if_52_5_206_5_end
        if_52_8_206_5_code:
;           [52:17] return
            jmp bar_206_5_end
        if_52_5_206_5_end:
;       [53:5] arg = 0xff
;       [53:11] 0xff
        mov qword [rbp + 336], 255
    bar_206_5_end:
;   [207:5] assert(i == 0)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? i == 0
;   [207:12] ? i == 0
    cmp_207_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_207_12:
;   [36:6] assert(x : bool)
    assert_207_5:
;       [207:5] alias x -> r15b
        if_36_26_207_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_207_5:
        cmp r15b, 0
        jne if_36_23_207_5_end
        if_36_26_207_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_207_5_end:
;       [207:5] free scratch register r15
    assert_207_5_end:
;   [209:5] i = 1
;   [209:9] 1
    mov qword [rbp + 336], 1
;   [210:5] bar(i)
;   [51:6] bar(arg)
    bar_210_5:
;       [210:5] alias arg -> i
        if_52_8_210_5:
;       [52:8] ? arg == 0
;       [52:8] ? arg == 0
        cmp_52_8_210_5:
        cmp qword [rbp + 336], 0
        jne if_52_5_210_5_end
        if_52_8_210_5_code:
;           [52:17] return
            jmp bar_210_5_end
        if_52_5_210_5_end:
;       [53:5] arg = 0xff
;       [53:11] 0xff
        mov qword [rbp + 336], 255
    bar_210_5_end:
;   [211:5] assert(i == 0xff)
;   [211:12] allocate scratch register -> r15
;   [211:12] ? i == 0xff
;   [211:12] ? i == 0xff
    cmp_211_12:
    cmp qword [rbp + 336], 255
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_211_5_end:
;       [211:5] free scratch register r15
    assert_211_5_end:
;   [213:5] var j = 1
;   [213:9] j: i64 (8 B @ [rbp + 344])
;   [213:9] j = 1
;   [213:13] 1
    mov qword [rbp + 344], 1
;   [214:5] var k = baz(j)
;   [214:9] k: i64 (8 B @ [rbp + 352])
;   [214:9] k = baz(j)
;   [214:13] k = baz(j)
;   [214:13] = expression
;   [214:13] baz(j)
;   [64:6] baz(arg) : i64 res
    baz_214_13:
;       [214:13] alias res -> k
;       [214:13] alias arg -> j
;       [65:5] res = arg * 2
;       [65:11] instructions without scratch register 5, with 3
;       [65:11] allocate scratch register -> r15
;       [65:11] arg
        mov r15, qword [rbp + 344]
;       [65:17] r15 * 2
;       [65:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [65:11] free scratch register r15
    baz_214_13_end:
;   [215:5] assert(k == 2)
;   [215:12] allocate scratch register -> r15
;   [215:12] ? k == 2
;   [215:12] ? k == 2
    cmp_215_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_215_12:
;   [36:6] assert(x : bool)
    assert_215_5:
;       [215:5] alias x -> r15b
        if_36_26_215_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_215_5:
        cmp r15b, 0
        jne if_36_23_215_5_end
        if_36_26_215_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_215_5_end:
;       [215:5] free scratch register r15
    assert_215_5_end:
;   [217:5] k = baz(1)
;   [217:9] k = baz(1)
;   [217:9] = expression
;   [217:9] baz(1)
;   [64:6] baz(arg) : i64 res
    baz_217_9:
;       [217:9] alias res -> k
;       [217:9] alias arg -> 1
;       [65:5] res = arg * 2
;       [65:11] instructions without scratch register 4, with 3
;       [65:11] allocate scratch register -> r15
;       [65:11] arg
        mov r15, 1
;       [65:17] r15 * 2
;       [65:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [65:11] free scratch register r15
    baz_217_9_end:
;   [218:5] assert(k == 2)
;   [218:12] allocate scratch register -> r15
;   [218:12] ? k == 2
;   [218:12] ? k == 2
    cmp_218_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_218_12:
;   [36:6] assert(x : bool)
    assert_218_5:
;       [218:5] alias x -> r15b
        if_36_26_218_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_218_5:
        cmp r15b, 0
        jne if_36_23_218_5_end
        if_36_26_218_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_218_5_end:
;       [218:5] free scratch register r15
    assert_218_5_end:
;   [220:5] var p0 : point = {baz(3), 0}
;   [220:9] p0: point (16 B @ [rbp + 360])
;   [220:9] p0 = {baz(3), 0}
;   [220:23] copy field 'x'
;   [220:23] p0.x = baz(3)
;   [220:23] = expression
;   [220:23] baz(3)
;   [64:6] baz(arg) : i64 res
    baz_220_23:
;       [220:23] alias res -> p0.x (lea: rbp + 360)
;       [220:23] alias arg -> 3
;       [65:5] res = arg * 2
;       [65:11] instructions without scratch register 4, with 3
;       [65:11] allocate scratch register -> r15
;       [65:11] arg
        mov r15, 3
;       [65:17] r15 * 2
;       [65:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 360], r15
;       [65:11] free scratch register r15
    baz_220_23_end:
;   [220:31] copy field 'y'
    mov qword [rbp + 368], 0
;   [221:5] assert(p0.x == 6)
;   [221:12] allocate scratch register -> r15
;   [221:12] ? p0.x == 6
;   [221:12] ? p0.x == 6
    cmp_221_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_221_12:
;   [36:6] assert(x : bool)
    assert_221_5:
;       [221:5] alias x -> r15b
        if_36_26_221_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_221_5:
        cmp r15b, 0
        jne if_36_23_221_5_end
        if_36_26_221_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_221_5_end:
;       [221:5] free scratch register r15
    assert_221_5_end:
;   [223:5] var pt : point = point_init()
;   [223:9] pt: point (16 B @ [rbp + 376])
;   [223:9] pt = point_init()
;   [223:22] point_init()
;   [83:6] point_init() : point res
    point_init_223_22:
;       [223:22] alias res -> pt
;       [84:5] res.x = -1
;       [84:14] -1
        mov qword [rbp + 376], -1
;       [85:5] res.y = -2
;       [85:14] -2
        mov qword [rbp + 384], -2
    point_init_223_22_end:
;   [224:5] assert(pt.x == -1)
;   [224:12] allocate scratch register -> r15
;   [224:12] ? pt.x == -1
;   [224:12] ? pt.x == -1
    cmp_224_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_224_12:
;   [36:6] assert(x : bool)
    assert_224_5:
;       [224:5] alias x -> r15b
        if_36_26_224_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_224_5:
        cmp r15b, 0
        jne if_36_23_224_5_end
        if_36_26_224_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_224_5_end:
;       [224:5] free scratch register r15
    assert_224_5_end:
;   [225:5] assert(pt.y == -2)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? pt.y == -2
;   [225:12] ? pt.y == -2
    cmp_225_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_225_12:
;   [36:6] assert(x : bool)
    assert_225_5:
;       [225:5] alias x -> r15b
        if_36_26_225_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_225_5:
        cmp r15b, 0
        jne if_36_23_225_5_end
        if_36_26_225_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_225_5_end:
;       [225:5] free scratch register r15
    assert_225_5_end:
;   [227:5] var x = 1
;   [227:9] x: i64 (8 B @ [rbp + 392])
;   [227:9] x = 1
;   [227:13] 1
    mov qword [rbp + 392], 1
;   [228:5] var y = 2
;   [228:9] y: i64 (8 B @ [rbp + 400])
;   [228:9] y = 2
;   [228:13] 2
    mov qword [rbp + 400], 2
;   [230:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [230:9] o1: object (20 B @ [rbp + 408])
;   [230:9] o1 = {{x * 10, y}, 0xff0000}
;   [230:24] copy field 'pos'
;   [230:25] copy field 'x'
;   [230:25] instructions without scratch register 5, with 3
;   [230:25] allocate scratch register -> r15
;   [230:25] x
    mov r15, qword [rbp + 392]
;   [230:29] r15 * 10
;   [230:29] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 408], r15
;   [230:25] free scratch register r15
;   [230:33] copy field 'y'
;   [230:33] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
;   [230:33] free scratch register r15
;   [230:37] copy field 'color'
    mov dword [rbp + 424], 16711680
;   [231:5] assert(o1.pos.x == 10)
;   [231:12] allocate scratch register -> r15
;   [231:12] ? o1.pos.x == 10
;   [231:12] ? o1.pos.x == 10
    cmp_231_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_231_12:
;   [36:6] assert(x : bool)
    assert_231_5:
;       [231:5] alias x -> r15b
        if_36_26_231_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_231_5:
        cmp r15b, 0
        jne if_36_23_231_5_end
        if_36_26_231_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_231_5_end:
;       [231:5] free scratch register r15
    assert_231_5_end:
;   [232:5] assert(o1.pos.y == 2)
;   [232:12] allocate scratch register -> r15
;   [232:12] ? o1.pos.y == 2
;   [232:12] ? o1.pos.y == 2
    cmp_232_12:
    cmp qword [rbp + 416], 2
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_232_5_end:
;       [232:5] free scratch register r15
    assert_232_5_end:
;   [233:5] assert(o1.color == 0xff0000)
;   [233:12] allocate scratch register -> r15
;   [233:12] ? o1.color == 0xff0000
;   [233:12] ? o1.color == 0xff0000
    cmp_233_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_233_12:
;   [36:6] assert(x : bool)
    assert_233_5:
;       [233:5] alias x -> r15b
        if_36_26_233_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_233_5:
        cmp r15b, 0
        jne if_36_23_233_5_end
        if_36_26_233_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_233_5_end:
;       [233:5] free scratch register r15
    assert_233_5_end:
;   [235:5] var p1 : point = {-x, -y}
;   [235:9] p1: point (16 B @ [rbp + 428])
;   [235:9] p1 = {-x, -y}
;   [235:23] copy field 'x'
;   [235:23] allocate scratch register -> r15
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
;   [235:23] free scratch register r15
    neg qword [rbp + 428]
;   [235:27] copy field 'y'
;   [235:27] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
;   [235:27] free scratch register r15
    neg qword [rbp + 436]
;   [236:5] o1.pos = p1
;   [236:14] size <= 16 B, use mov
;   [236:14] allocate named register rax
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
;   [236:14] free named register rax
;   [237:5] assert(o1.pos.x == -1)
;   [237:12] allocate scratch register -> r15
;   [237:12] ? o1.pos.x == -1
;   [237:12] ? o1.pos.x == -1
    cmp_237_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_237_12:
;   [36:6] assert(x : bool)
    assert_237_5:
;       [237:5] alias x -> r15b
        if_36_26_237_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_237_5:
        cmp r15b, 0
        jne if_36_23_237_5_end
        if_36_26_237_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_237_5_end:
;       [237:5] free scratch register r15
    assert_237_5_end:
;   [238:5] assert(o1.pos.y == -2)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? o1.pos.y == -2
;   [238:12] ? o1.pos.y == -2
    cmp_238_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_238_12:
;   [36:6] assert(x : bool)
    assert_238_5:
;       [238:5] alias x -> r15b
        if_36_26_238_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_238_5:
        cmp r15b, 0
        jne if_36_23_238_5_end
        if_36_26_238_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_238_5_end:
;       [238:5] free scratch register r15
    assert_238_5_end:
;   [240:5] var o2 : object = o1
;   [240:9] o2: object (20 B @ [rbp + 444])
;   [240:9] o2 = o1
;   [240:23] allocate named register rsi
;   [240:23] allocate named register rdi
;   [240:23] allocate named register rcx
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
;   [240:23] free named register rcx
;   [240:23] free named register rdi
;   [240:23] free named register rsi
;   [241:5] assert(o2.pos.x == -1)
;   [241:12] allocate scratch register -> r15
;   [241:12] ? o2.pos.x == -1
;   [241:12] ? o2.pos.x == -1
    cmp_241_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_241_12:
;   [36:6] assert(x : bool)
    assert_241_5:
;       [241:5] alias x -> r15b
        if_36_26_241_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_241_5:
        cmp r15b, 0
        jne if_36_23_241_5_end
        if_36_26_241_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_241_5_end:
;       [241:5] free scratch register r15
    assert_241_5_end:
;   [242:5] assert(o2.pos.y == -2)
;   [242:12] allocate scratch register -> r15
;   [242:12] ? o2.pos.y == -2
;   [242:12] ? o2.pos.y == -2
    cmp_242_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_242_12:
;   [36:6] assert(x : bool)
    assert_242_5:
;       [242:5] alias x -> r15b
        if_36_26_242_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_242_5:
        cmp r15b, 0
        jne if_36_23_242_5_end
        if_36_26_242_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_242_5_end:
;       [242:5] free scratch register r15
    assert_242_5_end:
;   [243:5] assert(o2.color == 0xff0000)
;   [243:12] allocate scratch register -> r15
;   [243:12] ? o2.color == 0xff0000
;   [243:12] ? o2.color == 0xff0000
    cmp_243_12:
    cmp dword [rbp + 460], 16711680
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_243_5_end:
;       [243:5] free scratch register r15
    assert_243_5_end:
;   [245:5] var o3 : object[2]
;   [245:9] o3: object[2] (40 B @ [rbp + 464])
;   [245:9] zero 2 * 20 B = 40 B
;   [245:5] allocate named register rax
;   [245:5] allocate named register rdi
;   [245:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
;   [245:5] free named register rcx
;   [245:5] free named register rdi
;   [245:5] free named register rax
;   [246:5] o3.pos.y = 73
;   [246:16] 73
    mov qword [rbp + 472], 73
;   [247:5] # index 0 in an array can be accessed without array index
;   [249:5] assert(o3[0].pos.y == 73)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? o3[0].pos.y == 73
;   [249:12] ? o3[0].pos.y == 73
    cmp_249_12:
;   [249:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [249:12] allocate scratch register -> r13
;   [249:15] set array index
;   [249:15] 0
    mov r13, 0
;   [249:15] bounds check
;   [249:15] allocate scratch register -> r12
;   [249:15] line number
    mov r12, 249
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [249:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [249:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [249:12] free scratch register r14
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_249_5_end:
;       [249:5] free scratch register r15
    assert_249_5_end:
;   [251:5] o3[1] = object_init()
;   [251:5] allocate scratch register -> r15
    lea r15, [rbp + 464]
;   [251:5] allocate scratch register -> r14
;   [251:8] set array index
;   [251:8] 1
    mov r14, 1
;   [251:8] bounds check
;   [251:8] allocate scratch register -> r13
;   [251:8] line number
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [251:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [251:5] free scratch register r14
;   [251:13] object_init()
;   [88:6] object_init() : object res
    object_init_251_13:
;       [251:13] alias res -> o3 (lea: r15)
;       [89:5] res.pos.y = 74
;       [89:17] 74
        mov qword [r15 + 8], 74
    object_init_251_13_end:
;   [251:5] free scratch register r15
;   [252:5] assert(o3[1].pos.y == 74)
;   [252:12] allocate scratch register -> r15
;   [252:12] ? o3[1].pos.y == 74
;   [252:12] ? o3[1].pos.y == 74
    cmp_252_12:
;   [252:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [252:12] allocate scratch register -> r13
;   [252:15] set array index
;   [252:15] 1
    mov r13, 1
;   [252:15] bounds check
;   [252:15] allocate scratch register -> r12
;   [252:15] line number
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [252:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [252:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [252:12] free scratch register r14
    sete r15b
    bool_end_252_12:
;   [36:6] assert(x : bool)
    assert_252_5:
;       [252:5] alias x -> r15b
        if_36_26_252_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_252_5:
        cmp r15b, 0
        jne if_36_23_252_5_end
        if_36_26_252_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_252_5_end:
;       [252:5] free scratch register r15
    assert_252_5_end:
;   [254:5] var worlds : world[8]
;   [254:9] worlds: world[8] (512 B @ [rbp + 504])
;   [254:9] zero 8 * 64 B = 512 B
;   [254:5] allocate named register rax
;   [254:5] allocate named register rdi
;   [254:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
;   [254:5] free named register rcx
;   [254:5] free named register rdi
;   [254:5] free named register rax
;   [255:5] worlds[1].locations[1] = 0xffee
;   [255:5] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [255:5] allocate scratch register -> r14
;   [255:12] set array index
;   [255:12] 1
    mov r14, 1
;   [255:12] bounds check
;   [255:12] allocate scratch register -> r13
;   [255:12] line number
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [255:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [255:5] free scratch register r14
;   [255:5] allocate scratch register -> r14
;   [255:25] set array index
;   [255:25] 1
    mov r14, 1
;   [255:25] bounds check
;   [255:25] allocate scratch register -> r13
;   [255:25] line number
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [255:25] free scratch register r13
;   [255:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [255:5] free scratch register r14
;   [255:5] free scratch register r15
;   [256:5] assert(worlds[1].locations[1] == 0xffee)
;   [256:12] allocate scratch register -> r15
;   [256:12] ? worlds[1].locations[1] == 0xffee
;   [256:12] ? worlds[1].locations[1] == 0xffee
    cmp_256_12:
;   [256:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [256:12] allocate scratch register -> r13
;   [256:19] set array index
;   [256:19] 1
    mov r13, 1
;   [256:19] bounds check
;   [256:19] allocate scratch register -> r12
;   [256:19] line number
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [256:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [256:12] free scratch register r13
;   [256:12] allocate scratch register -> r13
;   [256:32] set array index
;   [256:32] 1
    mov r13, 1
;   [256:32] bounds check
;   [256:32] allocate scratch register -> r12
;   [256:32] line number
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [256:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [256:12] free scratch register r13
;   [256:12] free scratch register r14
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_256_5_end:
;       [256:5] free scratch register r15
    assert_256_5_end:
;   [258:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [258:5] allocate named register rsi
;   [258:5] allocate named register rdi
;   [258:5] allocate named register rcx
;   [261:9] array_size_of(worlds.locations)
;   [261:9] rcx = array_size_of(worlds.locations)
;   [261:9] = expression
;   [261:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [259:9] worlds[1].locations
;   [259:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [259:9] allocate scratch register -> r14
;   [259:16] set array index
;   [259:16] 1
    mov r14, 1
;   [259:16] bounds check
;   [259:16] allocate scratch register -> r13
;   [259:16] line number
    mov r13, 259
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [259:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [259:9] free scratch register r14
;   [259:9] bounds check
;   [259:9] allocate scratch register -> r14
;   [259:9] line number
    mov r14, 259
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [259:9] free scratch register r14
    lea rsi, [r15]
;   [258:5] free scratch register r15
;   [260:9] worlds[0].locations
;   [260:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [260:9] allocate scratch register -> r14
;   [260:16] set array index
;   [260:16] 0
    mov r14, 0
;   [260:16] bounds check
;   [260:16] allocate scratch register -> r13
;   [260:16] line number
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [260:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [260:9] free scratch register r14
;   [260:9] bounds check
;   [260:9] allocate scratch register -> r14
;   [260:9] line number
    mov r14, 260
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [260:9] free scratch register r14
    lea rdi, [r15]
;   [258:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [258:5] free named register rcx
;   [258:5] free named register rdi
;   [258:5] free named register rsi
;   [263:5] # `array_copy` is built-in and can use indexed positions
;   [264:5] # `array_size_of` is built-in
;   [266:5] assert(worlds[0].locations[1] == 0xffee)
;   [266:12] allocate scratch register -> r15
;   [266:12] ? worlds[0].locations[1] == 0xffee
;   [266:12] ? worlds[0].locations[1] == 0xffee
    cmp_266_12:
;   [266:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [266:12] allocate scratch register -> r13
;   [266:19] set array index
;   [266:19] 0
    mov r13, 0
;   [266:19] bounds check
;   [266:19] allocate scratch register -> r12
;   [266:19] line number
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [266:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [266:12] free scratch register r13
;   [266:12] allocate scratch register -> r13
;   [266:32] set array index
;   [266:32] 1
    mov r13, 1
;   [266:32] bounds check
;   [266:32] allocate scratch register -> r12
;   [266:32] line number
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [266:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [266:12] free scratch register r13
;   [266:12] free scratch register r14
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_266_5_end:
;       [266:5] free scratch register r15
    assert_266_5_end:
;   [267:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [267:12] allocate scratch register -> r15
;   [267:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [267:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_267_12:
;   [267:12] allocate scratch register -> r14
;       [267:12] r14b = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [267:12] = expression
;       [267:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [267:12] allocate named register rsi
;       [267:12] allocate named register rdi
;       [267:12] allocate named register rcx
;       [270:14] array_size_of(worlds.locations)
;       [270:14] rcx = array_size_of(worlds.locations)
;       [270:14] = expression
;       [270:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [268:14] worlds[0].locations
;       [268:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [268:14] allocate scratch register -> r12
;       [268:21] set array index
;       [268:21] 0
        mov r12, 0
;       [268:21] bounds check
;       [268:21] allocate scratch register -> r10
;       [268:21] line number
        mov r10, 268
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
;       [268:21] free scratch register r10
        shl r12, 6
        add r13, r12
;       [268:14] free scratch register r12
;       [268:14] bounds check
;       [268:14] allocate scratch register -> r12
;       [268:14] line number
        mov r12, 268
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [268:14] free scratch register r12
        lea rsi, [r13]
;       [267:12] free scratch register r13
;       [269:14] worlds[1].locations
;       [269:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [269:14] allocate scratch register -> r12
;       [269:21] set array index
;       [269:21] 1
        mov r12, 1
;       [269:21] bounds check
;       [269:21] allocate scratch register -> r10
;       [269:21] line number
        mov r10, 269
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
;       [269:21] free scratch register r10
        shl r12, 6
        add r13, r12
;       [269:14] free scratch register r12
;       [269:14] bounds check
;       [269:14] allocate scratch register -> r12
;       [269:14] line number
        mov r12, 269
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [269:14] free scratch register r12
        lea rdi, [r13]
;       [267:12] free scratch register r13
        shl rcx, 3
        repe cmpsb
;       [267:12] free named register rcx
;       [267:12] free named register rdi
;       [267:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [267:12] free scratch register r14
    setne r15b
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_267_5_end:
;       [267:5] free scratch register r15
    assert_267_5_end:
;   [273:5] var arr2 : i64[] = { -1, 2 }
;   [273:9] arr2: i64[2] (16 B @ [rbp + 1016])
;   [273:9] arr2 = { -1, 2 }
;   [273:26] [0]
;   [273:27] -1
    mov qword [rbp + 1016], -1
;   [273:26] [1]
;   [273:30] 2
    mov qword [rbp + 1024], 2
;   [274:5] assert(array_size_of(arr2) == 2)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? array_size_of(arr2) == 2
;   [274:12] ? array_size_of(arr2) == 2
    cmp_274_12:
;   [274:12] allocate scratch register -> r14
;       [274:12] r14 = array_size_of(arr2)
;       [274:12] = expression
;       [274:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
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
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_274_5_end:
;       [274:5] free scratch register r15
    assert_274_5_end:
;   [275:5] assert(arr2[0] == -1)
;   [275:12] allocate scratch register -> r15
;   [275:12] ? arr2[0] == -1
;   [275:12] ? arr2[0] == -1
    cmp_275_12:
;   [275:12] allocate scratch register -> r14
;   [275:17] set array index
;   [275:17] 0
    mov r14, 0
;   [275:17] bounds check
;   [275:17] allocate scratch register -> r13
;   [275:17] line number
    mov r13, 275
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [275:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], -1
;   [275:12] free scratch register r14
    sete r15b
    bool_end_275_12:
;   [36:6] assert(x : bool)
    assert_275_5:
;       [275:5] alias x -> r15b
        if_36_26_275_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_275_5:
        cmp r15b, 0
        jne if_36_23_275_5_end
        if_36_26_275_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_275_5_end:
;       [275:5] free scratch register r15
    assert_275_5_end:
;   [276:5] assert(arr2[1] == 2)
;   [276:12] allocate scratch register -> r15
;   [276:12] ? arr2[1] == 2
;   [276:12] ? arr2[1] == 2
    cmp_276_12:
;   [276:12] allocate scratch register -> r14
;   [276:17] set array index
;   [276:17] 1
    mov r14, 1
;   [276:17] bounds check
;   [276:17] allocate scratch register -> r13
;   [276:17] line number
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [276:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], 2
;   [276:12] free scratch register r14
    sete r15b
    bool_end_276_12:
;   [36:6] assert(x : bool)
    assert_276_5:
;       [276:5] alias x -> r15b
        if_36_26_276_5:
;       [36:26] ? not x
;       [36:26] ? not x
        cmp_36_26_276_5:
        cmp r15b, 0
        jne if_36_23_276_5_end
        if_36_26_276_5_code:
;           [36:32] exit(1)
;           [36:32] allocate named register rdi
;           [36:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [36:32] free named register rdi
        if_36_23_276_5_end:
;       [276:5] free scratch register r15
    assert_276_5_end:
;   [278:5] var counter
;   [278:9] counter: i64 (8 B @ [rbp + 1032])
;   [278:9] zero 1 * 8 B = 8 B
;   [278:5] size <= 32 B, use mov
    mov qword [rbp + 1032], 0
;   [279:5] var nm : str
;   [279:9] nm: str (128 B @ [rbp + 1040])
;   [279:9] zero 1 * 128 B = 128 B
;   [279:5] allocate named register rax
;   [279:5] allocate named register rdi
;   [279:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
;   [279:5] free named register rcx
;   [279:5] free named register rdi
;   [279:5] free named register rax
;   [280:5] print(hello)
;   [38:6] print(str : i8[])
    print_280_5:
;       [280:5] alias str -> hello
;       [39:5] write(1, address_of(str), array_size_of(str))
;       [39:5] allocate named register rdi
;       [39:11] 1
        mov rdi, 1
;       [39:5] allocate named register rsi
;       [39:14] rsi = address_of(str)
;       [39:14] = expression
;       [39:14] address_of(str)
        lea rsi, [rbp]
;       [39:5] allocate named register rdx
;       [39:31] rdx = array_size_of(str)
;       [39:31] = expression
;       [39:31] array_size_of(str)
        mov rdx, 21
;       [39:5] allocate named register rax
        mov rax, 1
        syscall
;       [39:5] free named register rax
;       [39:5] free named register rdx
;       [39:5] free named register rsi
;       [39:5] free named register rdi
    print_280_5_end:
;   [281:5] label
    loop_281_5:
;       [282:9] counter = counter + 1
;       [282:19] instructions without scratch register 1, with 3
;       [282:19] counter
;       [282:29] counter + 1
        add qword [rbp + 1032], 1
;       [283:9] print_num(counter)
;       [283:9] address of argument 'counter' to parameter 'num'
;       [283:9] allocate scratch register -> r15
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
;       [283:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
;       [284:9] print(colon)
;       [38:6] print(str : i8[])
        print_284_9:
;           [284:9] alias str -> colon
;           [39:5] write(1, address_of(str), array_size_of(str))
;           [39:5] allocate named register rdi
;           [39:11] 1
            mov rdi, 1
;           [39:5] allocate named register rsi
;           [39:14] rsi = address_of(str)
;           [39:14] = expression
;           [39:14] address_of(str)
            lea rsi, [rbp + 61]
;           [39:5] allocate named register rdx
;           [39:31] rdx = array_size_of(str)
;           [39:31] = expression
;           [39:31] array_size_of(str)
            mov rdx, 2
;           [39:5] allocate named register rax
            mov rax, 1
            syscall
;           [39:5] free named register rax
;           [39:5] free named register rdx
;           [39:5] free named register rsi
;           [39:5] free named register rdi
        print_284_9_end:
;       [285:9] print(prompt1)
;       [38:6] print(str : i8[])
        print_285_9:
;           [285:9] alias str -> prompt1
;           [39:5] write(1, address_of(str), array_size_of(str))
;           [39:5] allocate named register rdi
;           [39:11] 1
            mov rdi, 1
;           [39:5] allocate named register rsi
;           [39:14] rsi = address_of(str)
;           [39:14] = expression
;           [39:14] address_of(str)
            lea rsi, [rbp + 21]
;           [39:5] allocate named register rdx
;           [39:31] rdx = array_size_of(str)
;           [39:31] = expression
;           [39:31] array_size_of(str)
            mov rdx, 12
;           [39:5] allocate named register rax
            mov rax, 1
            syscall
;           [39:5] free named register rax
;           [39:5] free named register rdx
;           [39:5] free named register rsi
;           [39:5] free named register rdi
        print_285_9_end:
;       [286:9] str_in(nm)
;       [74:6] str_in(s : str)
        str_in_286_9:
;           [286:9] alias s -> nm
;           [75:5] var nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [75:9] nbytes: i64 (8 B @ [rbp + 1168])
;           [75:9] nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [75:18] nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [75:18] = expression
;           [75:18] read(0, address_of(s.data), array_size_of(s.data))
;           [75:18] allocate named register rdi
;           [75:23] 0
            mov rdi, 0
;           [75:18] allocate named register rsi
;           [75:26] rsi = address_of(s.data)
;           [75:26] = expression
;           [75:26] address_of(s.data)
            lea rsi, [rbp + 1041]
;           [75:18] allocate named register rdx
;           [75:46] rdx = array_size_of(s.data)
;           [75:46] = expression
;           [75:46] array_size_of(s.data)
            mov rdx, 127
;           [75:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
;           [75:18] free named register rax
;           [75:18] free named register rdx
;           [75:18] free named register rsi
;           [75:18] free named register rdi
;           [76:5] s.len = nbytes - 1
;           [76:13] instructions without scratch register 3, with 3
;           [76:13] nbytes
;           [76:13] allocate scratch register -> r15
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
;           [76:13] free scratch register r15
;           [76:22] s.len - 1
            sub byte [rbp + 1040], 1
        str_in_286_9_end:
        if_287_12:
;       [287:12] ? nm.len == 0
;       [287:12] ? nm.len == 0
        cmp_287_12:
        cmp byte [rbp + 1040], 0
        jne if_289_19
        if_287_12_code:
;           [288:13] break
            jmp loop_281_5_end
        jmp if_287_9_end
        if_289_19:
;       [289:19] ? nm.len <= 4
;       [289:19] ? nm.len <= 4
        cmp_289_19:
        cmp byte [rbp + 1040], 4
        jg if_else_287_9
        if_289_19_code:
;           [290:13] print(prompt2)
;           [38:6] print(str : i8[])
            print_290_13:
;               [290:13] alias str -> prompt2
;               [39:5] write(1, address_of(str), array_size_of(str))
;               [39:5] allocate named register rdi
;               [39:11] 1
                mov rdi, 1
;               [39:5] allocate named register rsi
;               [39:14] rsi = address_of(str)
;               [39:14] = expression
;               [39:14] address_of(str)
                lea rsi, [rbp + 33]
;               [39:5] allocate named register rdx
;               [39:31] rdx = array_size_of(str)
;               [39:31] = expression
;               [39:31] array_size_of(str)
                mov rdx, 20
;               [39:5] allocate named register rax
                mov rax, 1
                syscall
;               [39:5] free named register rax
;               [39:5] free named register rdx
;               [39:5] free named register rsi
;               [39:5] free named register rdi
            print_290_13_end:
;           [291:13] continue
            jmp loop_281_5
        jmp if_287_9_end
        if_else_287_9:
;           [293:13] print(prompt3)
;           [38:6] print(str : i8[])
            print_293_13:
;               [293:13] alias str -> prompt3
;               [39:5] write(1, address_of(str), array_size_of(str))
;               [39:5] allocate named register rdi
;               [39:11] 1
                mov rdi, 1
;               [39:5] allocate named register rsi
;               [39:14] rsi = address_of(str)
;               [39:14] = expression
;               [39:14] address_of(str)
                lea rsi, [rbp + 53]
;               [39:5] allocate named register rdx
;               [39:31] rdx = array_size_of(str)
;               [39:31] = expression
;               [39:31] array_size_of(str)
                mov rdx, 6
;               [39:5] allocate named register rax
                mov rax, 1
                syscall
;               [39:5] free named register rax
;               [39:5] free named register rdx
;               [39:5] free named register rsi
;               [39:5] free named register rdi
            print_293_13_end:
;           [294:13] str_out(nm)
;           [79:6] str_out(s : str)
            str_out_294_13:
;               [294:13] alias s -> nm
;               [80:5] write(1, address_of(s.data), s.len)
;               [80:5] allocate named register rdi
;               [80:11] 1
                mov rdi, 1
;               [80:5] allocate named register rsi
;               [80:14] rsi = address_of(s.data)
;               [80:14] = expression
;               [80:14] address_of(s.data)
                lea rsi, [rbp + 1041]
;               [80:5] allocate named register rdx
;               [80:34] s.len
                movsx rdx, byte [rbp + 1040]
;               [80:5] allocate named register rax
                mov rax, 1
                syscall
;               [80:5] free named register rax
;               [80:5] free named register rdx
;               [80:5] free named register rsi
;               [80:5] free named register rdi
            str_out_294_13_end:
;           [295:13] print(dot)
;           [38:6] print(str : i8[])
            print_295_13:
;               [295:13] alias str -> dot
;               [39:5] write(1, address_of(str), array_size_of(str))
;               [39:5] allocate named register rdi
;               [39:11] 1
                mov rdi, 1
;               [39:5] allocate named register rsi
;               [39:14] rsi = address_of(str)
;               [39:14] = expression
;               [39:14] address_of(str)
                lea rsi, [rbp + 59]
;               [39:5] allocate named register rdx
;               [39:31] rdx = array_size_of(str)
;               [39:31] = expression
;               [39:31] array_size_of(str)
                mov rdx, 1
;               [39:5] allocate named register rax
                mov rax, 1
                syscall
;               [39:5] free named register rax
;               [39:5] free named register rdx
;               [39:5] free named register rsi
;               [39:5] free named register rdi
            print_295_13_end:
;           [296:13] print(nl)
;           [38:6] print(str : i8[])
            print_296_13:
;               [296:13] alias str -> nl
;               [39:5] write(1, address_of(str), array_size_of(str))
;               [39:5] allocate named register rdi
;               [39:11] 1
                mov rdi, 1
;               [39:5] allocate named register rsi
;               [39:14] rsi = address_of(str)
;               [39:14] = expression
;               [39:14] address_of(str)
                lea rsi, [rbp + 60]
;               [39:5] allocate named register rdx
;               [39:31] rdx = array_size_of(str)
;               [39:31] = expression
;               [39:31] array_size_of(str)
                mov rdx, 1
;               [39:5] allocate named register rax
                mov rax, 1
                syscall
;               [39:5] free named register rax
;               [39:5] free named register rdx
;               [39:5] free named register rsi
;               [39:5] free named register rdi
            print_296_13_end:
        if_287_9_end:
    jmp loop_281_5
    loop_281_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; 
;[100:15] noinline print_num(num)
print_num:
;   [100:25] num: i64 (8 B @ [rbx])
;   [101:5] var buf : i8[20]
;   [101:9] buf: i8[20] (20 B @ [rbx + 8])
;   [101:9] zero 20 * 1 B = 20 B
;   [101:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [102:5] var n = num
;   [102:9] n: i64 (8 B @ [rbx + 28])
;   [102:9] n = num
;   [102:13] num
;   [102:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [102:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
;   [102:13] free scratch register r14
;   [102:13] free scratch register r15
;   [103:5] var is_negative : bool = false
;   [103:9] is_negative: bool (1 B @ [rbx + 36])
;   [103:9] is_negative = false
    mov byte [rbx + 36], 0
    if_105_8:
;   [105:8] ? n < 0
;   [105:8] ? n < 0
    cmp_105_8:
    cmp qword [rbx + 28], 0
    jge if_105_5_end
    if_105_8_code:
;       [106:9] is_negative = true
        mov byte [rbx + 36], 1
;       [107:9] n = -n
;       [107:14] -n
        neg qword [rbx + 28]
    if_105_5_end:
;   [110:5] var i = 20
;   [110:9] i: i64 (8 B @ [rbx + 37])
;   [110:9] i = 20
;   [110:13] 20
    mov qword [rbx + 37], 20
;   [111:5] label
    loop_111_5:
;       [112:9] i = i - 1
;       [112:13] instructions without scratch register 1, with 3
;       [112:13] i
;       [112:17] i - 1
        sub qword [rbx + 37], 1
;       [113:9] var ascii = 48 + (n % 10)
;       [113:13] ascii: i64 (8 B @ [rbx + 45])
;       [113:13] ascii = 48 + (n % 10)
;       [113:21] instructions without scratch register 8, with 9
;       [113:21] 48
        mov qword [rbx + 45], 48
;       [113:27] ascii + (n % 10)
;       [113:27] allocate scratch register -> r15
;       [113:27] n
        mov r15, qword [rbx + 28]
;       [113:31] r15 % 10
;       [113:31] div const
;       [113:31] allocate named register rax
        mov rax, r15
;       [113:31] allocate named register rdx
        cqo
;       [113:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [113:31] free scratch register r14
        mov r15, rdx
;       [113:31] free named register rdx
;       [113:31] free named register rax
        add qword [rbx + 45], r15
;       [113:27] free scratch register r15
;       [114:9] # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
;       [115:9] buf[i] = ascii
;       [115:9] allocate scratch register -> r15
;       [115:13] set array index
;       [115:13] i
        mov r15, qword [rbx + 37]
;       [115:13] bounds check
;       [115:13] allocate scratch register -> r14
;       [115:13] line number
        mov r14, 115
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [115:13] free scratch register r14
;       [115:18] ascii
;       [115:18] allocate scratch register -> r14
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
;       [115:18] free scratch register r14
;       [115:9] free scratch register r15
;       [116:9] n = n / 10
;       [116:13] instructions without scratch register 5, with 7
;       [116:13] n
;       [116:17] n / 10
;       [116:17] div const
;       [116:17] allocate named register rax
        mov rax, qword [rbx + 28]
;       [116:17] allocate named register rdx
        cqo
;       [116:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [116:17] free scratch register r15
        mov qword [rbx + 28], rax
;       [116:17] free named register rdx
;       [116:17] free named register rax
        if_117_12:
;       [117:12] ? n == 0
;       [117:12] ? n == 0
        cmp_117_12:
        cmp qword [rbx + 28], 0
        jne if_117_9_end
        if_117_12_code:
;           [117:19] break
            jmp loop_111_5_end
        if_117_9_end:
    jmp loop_111_5
    loop_111_5_end:
    if_120_8:
;   [120:8] ? is_negative
;   [120:8] ? is_negative
    cmp_120_8:
    cmp byte [rbx + 36], 0
    je if_120_5_end
    if_120_8_code:
;       [121:9] i = i - 1
;       [121:13] instructions without scratch register 1, with 3
;       [121:13] i
;       [121:17] i - 1
        sub qword [rbx + 37], 1
;       [122:9] buf[i] = 45
;       [122:9] allocate scratch register -> r15
;       [122:13] set array index
;       [122:13] i
        mov r15, qword [rbx + 37]
;       [122:13] bounds check
;       [122:13] allocate scratch register -> r14
;       [122:13] line number
        mov r14, 122
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [122:13] free scratch register r14
;       [122:18] 45
        mov byte [rbx + r15 + 8], 45
;       [122:9] free scratch register r15
    if_120_5_end:
;   [125:5] var write_pos = 0
;   [125:9] write_pos: i64 (8 B @ [rbx + 45])
;   [125:9] write_pos = 0
;   [125:21] 0
    mov qword [rbx + 45], 0
;   [126:5] label
    loop_126_5:
;       [127:9] buf[write_pos] = buf[i]
;       [127:9] allocate scratch register -> r15
;       [127:13] set array index
;       [127:13] write_pos
        mov r15, qword [rbx + 45]
;       [127:13] bounds check
;       [127:13] allocate scratch register -> r14
;       [127:13] line number
        mov r14, 127
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [127:13] free scratch register r14
;       [127:26] buf[i]
;       [127:26] allocate scratch register -> r14
;       [127:30] set array index
;       [127:30] i
        mov r14, qword [rbx + 37]
;       [127:30] bounds check
;       [127:30] allocate scratch register -> r13
;       [127:30] line number
        mov r13, 127
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [127:30] free scratch register r13
;       [127:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [127:26] free scratch register r13
;       [127:26] free scratch register r14
;       [127:9] free scratch register r15
;       [128:9] write_pos = write_pos + 1
;       [128:21] instructions without scratch register 1, with 3
;       [128:21] write_pos
;       [128:33] write_pos + 1
        add qword [rbx + 45], 1
;       [129:9] i = i + 1
;       [129:13] instructions without scratch register 1, with 3
;       [129:13] i
;       [129:17] i + 1
        add qword [rbx + 37], 1
        if_130_12:
;       [130:12] ? i == 20
;       [130:12] ? i == 20
        cmp_130_12:
        cmp qword [rbx + 37], 20
        jne if_130_9_end
        if_130_12_code:
;           [130:20] break
            jmp loop_126_5_end
        if_130_9_end:
    jmp loop_126_5
    loop_126_5_end:
;   [133:5] write(1, address_of(buf), write_pos)
;   [133:5] allocate named register rdi
;   [133:11] 1
    mov rdi, 1
;   [133:5] allocate named register rsi
;   [133:14] rsi = address_of(buf)
;   [133:14] = expression
;   [133:14] address_of(buf)
    lea rsi, [rbx + 8]
;   [133:5] allocate named register rdx
;   [133:31] write_pos
    mov rdx, qword [rbx + 45]
;   [133:5] allocate named register rax
    mov rax, 1
    syscall
;   [133:5] free named register rax
;   [133:5] free named register rdx
;   [133:5] free named register rsi
;   [133:5] free named register rdi
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
;            max frames in use: 8
;              dat var padding: 1 B
;                max vars size: 952 B
;          optimization pass 1: 124
;          optimization pass 2: 0
```
