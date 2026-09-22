# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super-loop program with non-reentrant inlined functions
* limited support for non-inline functions
* target x86_64 and rv32i (running in QEMU)

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
C/C++ Header                    52           3116            984          11554
C++                              1             46              7            235
-------------------------------------------------------------------------------
SUM:                            53           3162            991          11789
-------------------------------------------------------------------------------
```

## Sample

```text
# user types are defined using keyword `type`

# built-in types are `i63`, `i32`, `i16`, `i8` and `bool`

# default type is used if ommitted (`i64` on x86_64 and 'i32' on rv32i)

type point {x, y}

type object {pos : point, color : i32}

type world { locations : [8] }

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
dat    nums : [4] = { 1 } # remaining elements are zeroed
dat    str1 : str = { 3 } # remaining fields are zeroed

# default is to inline functions

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

# default argument type is i64 on x86_64 and i32 on rv32i

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}

# return target is specified as a variable, in this case `res`

# return variable is a mutable reference to destination

func inv(i : i32) : res i32 {
    res = ~i
}

func baz(arg) : res {
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

func point_init() : res point {
    res.x = -1
    res.y = -2
}

func object_init() : res object {
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

    var arr3 : [] = { 3, 5 }
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

    var arr2 : [] = { -1, 2 }
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
    cmp_139_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_139_12:
    assert_139_5:
        if_34_26_139_5:
        cmp_34_26_139_5:
        cmp r15b, 0
        jne if_34_23_139_5_end
        if_34_26_139_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_139_5_end:
    assert_139_5_end:
    mov qword [rbp + 240], -1
    cmp_143_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_143_12:
    assert_143_5:
        if_34_26_143_5:
        cmp_34_26_143_5:
        cmp r15b, 0
        jne if_34_23_143_5_end
        if_34_26_143_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_143_5_end:
    assert_143_5_end:
        cmp_147_16:
        bool_end_147_16:
        mov r15b, 1
        assert_147_9:
            if_34_26_147_9:
            cmp_34_26_147_9:
            cmp r15b, 0
            jne if_34_23_147_9_end
            if_34_26_147_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_34_23_147_9_end:
        assert_147_9_end:
    cmp_150_12:
    bool_end_150_12:
    mov r15b, 1
    assert_150_5:
        if_34_26_150_5:
        cmp_34_26_150_5:
        cmp r15b, 0
        jne if_34_23_150_5_end
        if_34_26_150_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_150_5_end:
    assert_150_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 156
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_157_12:
    mov r14, 1
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_34_26_157_5:
        cmp_34_26_157_5:
        cmp r15b, 0
        jne if_34_23_157_5_end
        if_34_26_157_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_157_5_end:
    assert_157_5_end:
    cmp_158_12:
    mov r14, 2
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_34_26_158_5:
        cmp_34_26_158_5:
        cmp r15b, 0
        jne if_34_23_158_5_end
        if_34_26_158_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_158_5_end:
    assert_158_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 160
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 160
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_162_12:
    mov r14, 0
    mov r13, 162
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_162_12:
    assert_162_5:
        if_34_26_162_5:
        cmp_34_26_162_5:
        cmp r15b, 0
        jne if_34_23_162_5_end
        if_34_26_162_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_162_5_end:
    assert_162_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 165
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 165
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_166_12:
        mov rcx, 4
        mov r13, 166
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 166
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
    bool_end_166_12:
    assert_166_5:
        if_34_26_166_5:
        cmp_34_26_166_5:
        cmp r15b, 0
        jne if_34_23_166_5_end
        if_34_26_166_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_166_5_end:
    assert_166_5_end:
    mov r15, 2
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_170_12:
        mov rcx, 4
        mov r13, 170
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 170
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
    bool_end_170_12:
    assert_170_5:
        if_34_26_170_5:
        cmp_34_26_170_5:
        cmp r15b, 0
        jne if_34_23_170_5_end
        if_34_26_170_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_170_5_end:
    assert_170_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 173
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_173_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_173_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_174_12:
    mov r14, qword [rbp + 248]
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_174_12:
    assert_174_5:
        if_34_26_174_5:
        cmp_34_26_174_5:
        cmp r15b, 0
        jne if_34_23_174_5_end
        if_34_26_174_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_174_5_end:
    assert_174_5_end:
    faz_176_5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_176_5_end:
    cmp_177_12:
    mov r14, 1
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_34_26_177_5:
        cmp_34_26_177_5:
        cmp r15b, 0
        jne if_34_23_177_5_end
        if_34_26_177_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_177_5_end:
    assert_177_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_180_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_180_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_180_5
    foo_180_5_end:
    cmp_183_12:
    mov r14, 0
    mov r13, 183
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
    bool_end_183_12:
    assert_183_5:
        if_34_26_183_5:
        cmp_34_26_183_5:
        cmp r15b, 0
        jne if_34_23_183_5_end
        if_34_26_183_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_183_5_end:
    assert_183_5_end:
    cmp_184_12:
    mov r14, 1
    mov r13, 184
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
    bool_end_184_12:
    assert_184_5:
        if_34_26_184_5:
        cmp_34_26_184_5:
        cmp r15b, 0
        jne if_34_23_184_5_end
        if_34_26_184_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_184_5_end:
    assert_184_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_191_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_191_5_end:
    cmp_192_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_34_26_192_5:
        cmp_34_26_192_5:
        cmp r15b, 0
        jne if_34_23_192_5_end
        if_34_26_192_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_192_5_end:
    assert_192_5_end:
    cmp_193_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_193_12:
    assert_193_5:
        if_34_26_193_5:
        cmp_34_26_193_5:
        cmp r15b, 0
        jne if_34_23_193_5_end
        if_34_26_193_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_193_5_end:
    assert_193_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_196_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_196_12:
    assert_196_5:
        if_34_26_196_5:
        cmp_34_26_196_5:
        cmp r15b, 0
        jne if_34_23_196_5_end
        if_34_26_196_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_196_5_end:
    assert_196_5_end:
    mov qword [rbp + 320], 3
    cmp_201_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_34_26_201_5:
        cmp_34_26_201_5:
        cmp r15b, 0
        jne if_34_23_201_5_end
        if_34_26_201_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_201_5_end:
    assert_201_5_end:
    mov qword [rbp + 336], 0
    bar_204_5:
        if_50_8_204_5:
        cmp_50_8_204_5:
        cmp qword [rbp + 336], 0
        jne if_50_5_204_5_end
        if_50_8_204_5_code:
            jmp bar_204_5_end
        if_50_5_204_5_end:
        mov qword [rbp + 336], 255
    bar_204_5_end:
    cmp_205_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_34_26_205_5:
        cmp_34_26_205_5:
        cmp r15b, 0
        jne if_34_23_205_5_end
        if_34_26_205_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_205_5_end:
    assert_205_5_end:
    mov qword [rbp + 336], 1
    bar_208_5:
        if_50_8_208_5:
        cmp_50_8_208_5:
        cmp qword [rbp + 336], 0
        jne if_50_5_208_5_end
        if_50_8_208_5_code:
            jmp bar_208_5_end
        if_50_5_208_5_end:
        mov qword [rbp + 336], 255
    bar_208_5_end:
    cmp_209_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_209_12:
    assert_209_5:
        if_34_26_209_5:
        cmp_34_26_209_5:
        cmp r15b, 0
        jne if_34_23_209_5_end
        if_34_26_209_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_209_5_end:
    assert_209_5_end:
    mov qword [rbp + 344], 1
    baz_212_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_212_13_end:
    cmp_213_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_34_26_213_5:
        cmp_34_26_213_5:
        cmp r15b, 0
        jne if_34_23_213_5_end
        if_34_26_213_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_213_5_end:
    assert_213_5_end:
    baz_215_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_215_9_end:
    cmp_216_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_34_26_216_5:
        cmp_34_26_216_5:
        cmp r15b, 0
        jne if_34_23_216_5_end
        if_34_26_216_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_216_5_end:
    assert_216_5_end:
    baz_218_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_218_23_end:
    mov qword [rbp + 368], 0
    cmp_219_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_34_26_219_5:
        cmp_34_26_219_5:
        cmp r15b, 0
        jne if_34_23_219_5_end
        if_34_26_219_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_219_5_end:
    assert_219_5_end:
    point_init_221_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_221_22_end:
    cmp_222_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_34_26_222_5:
        cmp_34_26_222_5:
        cmp r15b, 0
        jne if_34_23_222_5_end
        if_34_26_222_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_222_5_end:
    assert_222_5_end:
    cmp_223_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_34_26_223_5:
        cmp_34_26_223_5:
        cmp r15b, 0
        jne if_34_23_223_5_end
        if_34_26_223_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_223_5_end:
    assert_223_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_229_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_34_26_229_5:
        cmp_34_26_229_5:
        cmp r15b, 0
        jne if_34_23_229_5_end
        if_34_26_229_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_34_26_230_5:
        cmp_34_26_230_5:
        cmp r15b, 0
        jne if_34_23_230_5_end
        if_34_26_230_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_230_5_end:
    assert_230_5_end:
    cmp_231_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_34_26_231_5:
        cmp_34_26_231_5:
        cmp r15b, 0
        jne if_34_23_231_5_end
        if_34_26_231_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_231_5_end:
    assert_231_5_end:
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
    cmp_235_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_34_26_235_5:
        cmp_34_26_235_5:
        cmp r15b, 0
        jne if_34_23_235_5_end
        if_34_26_235_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_235_5_end:
    assert_235_5_end:
    cmp_236_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_34_26_236_5:
        cmp_34_26_236_5:
        cmp r15b, 0
        jne if_34_23_236_5_end
        if_34_26_236_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_236_5_end:
    assert_236_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_239_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_34_26_239_5:
        cmp_34_26_239_5:
        cmp r15b, 0
        jne if_34_23_239_5_end
        if_34_26_239_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_239_5_end:
    assert_239_5_end:
    cmp_240_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_34_26_240_5:
        cmp_34_26_240_5:
        cmp r15b, 0
        jne if_34_23_240_5_end
        if_34_26_240_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_240_5_end:
    assert_240_5_end:
    cmp_241_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_241_12:
    assert_241_5:
        if_34_26_241_5:
        cmp_34_26_241_5:
        cmp r15b, 0
        jne if_34_23_241_5_end
        if_34_26_241_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_241_5_end:
    assert_241_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_247_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 247
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
    bool_end_247_12:
    assert_247_5:
        if_34_26_247_5:
        cmp_34_26_247_5:
        cmp r15b, 0
        jne if_34_23_247_5_end
        if_34_26_247_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_247_5_end:
    assert_247_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 249
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_249_13:
        mov qword [r15 + 8], 74
    object_init_249_13_end:
    cmp_250_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 250
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
    bool_end_250_12:
    assert_250_5:
        if_34_26_250_5:
        cmp_34_26_250_5:
        cmp r15b, 0
        jne if_34_23_250_5_end
        if_34_26_250_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_250_5_end:
    assert_250_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_254_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 254
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 254
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_254_12:
    assert_254_5:
        if_34_26_254_5:
        cmp_34_26_254_5:
        cmp r15b, 0
        jne if_34_23_254_5_end
        if_34_26_254_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_254_5_end:
    assert_254_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 257
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 258
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_264_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 264
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 264
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_264_12:
    assert_264_5:
        if_34_26_264_5:
        cmp_34_26_264_5:
        cmp r15b, 0
        jne if_34_23_264_5_end
        if_34_26_264_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_264_5_end:
    assert_264_5_end:
    cmp_265_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r10, 266
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 266
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r10, 267
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 267
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
    bool_end_265_12:
    assert_265_5:
        if_34_26_265_5:
        cmp_34_26_265_5:
        cmp r15b, 0
        jne if_34_23_265_5_end
        if_34_26_265_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_265_5_end:
    assert_265_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_272_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_34_26_272_5:
        cmp_34_26_272_5:
        cmp r15b, 0
        jne if_34_23_272_5_end
        if_34_26_272_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    mov r14, 0
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_34_26_273_5:
        cmp_34_26_273_5:
        cmp r15b, 0
        jne if_34_23_273_5_end
        if_34_26_273_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_273_5_end:
    assert_273_5_end:
    cmp_274_12:
    mov r14, 1
    mov r13, 274
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_34_26_274_5:
        cmp_34_26_274_5:
        cmp r15b, 0
        jne if_34_23_274_5_end
        if_34_26_274_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_274_5_end:
    assert_274_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_278_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_278_5_end:
    loop_279_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_282_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_282_9_end:
        print_283_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_283_9_end:
        str_in_284_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_284_9_end:
        if_285_12:
        cmp_285_12:
        cmp byte [rbp + 1040], 0
        jne if_287_19
        if_285_12_code:
            jmp loop_279_5_end
        jmp if_285_9_end
        if_287_19:
        cmp_287_19:
        cmp byte [rbp + 1040], 4
        jg if_else_285_9
        if_287_19_code:
            print_288_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_288_13_end:
            jmp loop_279_5
        jmp if_285_9_end
        if_else_285_9:
            print_291_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_291_13_end:
            str_out_292_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_292_13_end:
            print_293_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_293_13_end:
            print_294_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_294_13_end:
        if_285_9_end:
    jmp loop_279_5
    loop_279_5_end:
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
    if_103_8:
    cmp_103_8:
    cmp qword [rbx + 28], 0
    jge if_103_5_end
    if_103_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_103_5_end:
    mov qword [rbx + 37], 20
    loop_109_5:
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
        mov r14, 113
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
        if_115_12:
        cmp_115_12:
        cmp qword [rbx + 28], 0
        jne if_115_9_end
        if_115_12_code:
            jmp loop_109_5_end
        if_115_9_end:
    jmp loop_109_5
    loop_109_5_end:
    if_118_8:
    cmp_118_8:
    cmp byte [rbx + 36], 0
    je if_118_5_end
    if_118_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 120
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_118_5_end:
    mov qword [rbx + 45], 0
    loop_124_5:
        mov r15, qword [rbx + 45]
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 125
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
        if_128_12:
        cmp_128_12:
        cmp qword [rbx + 37], 20
        jne if_128_9_end
        if_128_12_code:
            jmp loop_124_5_end
        if_128_9_end:
    jmp loop_124_5
    loop_124_5_end:
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
;[5:1] # default type is used if ommitted (`i64` on x86_64 and 'i32' on rv32i)
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
;[27:1] dat nums : [4] = { 1 }
;[27:8] nums: i64[4] (32 B @ [rbp + 63])
;[27:27] # remaining elements are zeroed
;[28:1] dat str1 : str = { 3 }
;[28:8] str1: str (128 B @ [rbp + 95])
;[28:27] # remaining fields are zeroed
;[30:1] # default is to inline functions
;[32:1] # single statement blocks can ommit { ... }
;[40:1] # function arguments and return are equivalent to mutable references
;[47:1] # default argument type is i64 on x86_64 and i32 on rv32i
;[54:1] # return target is specified as a variable, in this case `res`
;[56:1] # return variable is a mutable reference to destination
;[66:1] # array arguments are declared with type and []
;[90:7] const yes = 1
;[91:7] const no = 0
;[92:7] const maybe = -1
;[94:1] # constants can be declared in any scope and shadow outer declarations
;[96:1] # limited support for non-inlined functions
; 
main:
;   [135:5] var arr : i32[4]
;   [135:9] arr: i32[4] (16 B @ [rbp + 224])
;   [135:9] zero 4 * 4 B = 16 B
;   [135:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [136:5] # arrays are initialized to 0
;   [138:5] var answer
;   [138:9] answer: i64 (8 B @ [rbp + 240])
;   [138:9] zero 1 * 8 B = 8 B
;   [138:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [139:5] assert(answer == 0)
;   [139:12] allocate scratch register -> r15
;   [139:12] ? answer == 0
;   [139:12] ? answer == 0
    cmp_139_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_139_12:
;   [34:6] assert(x : bool)
    assert_139_5:
;       [139:5] alias x -> r15b
        if_34_26_139_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_139_5:
        cmp r15b, 0
        jne if_34_23_139_5_end
        if_34_26_139_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_139_5_end:
;       [139:5] free scratch register r15
    assert_139_5_end:
;   [140:5] # variables without initializer are zeroed
;   [142:5] answer = maybe
;   [142:14] maybe
    mov qword [rbp + 240], -1
;   [143:5] assert(answer == -1)
;   [143:12] allocate scratch register -> r15
;   [143:12] ? answer == -1
;   [143:12] ? answer == -1
    cmp_143_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_143_12:
;   [34:6] assert(x : bool)
    assert_143_5:
;       [143:5] alias x -> r15b
        if_34_26_143_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_143_5:
        cmp r15b, 0
        jne if_34_23_143_5_end
        if_34_26_143_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_143_5_end:
;       [143:5] free scratch register r15
    assert_143_5_end:
;       [146:15] const maybe = 33
;       [147:9] assert(maybe == 33)
;       [147:16] allocate scratch register -> r15
;       [147:16] ? maybe == 33
;       [147:16] ? maybe == 33
        cmp_147_16:
;       [147:16] const eval to true
        bool_end_147_16:
        mov r15b, 1
;       [34:6] assert(x : bool)
        assert_147_9:
;           [147:9] alias x -> r15b
            if_34_26_147_9:
;           [34:26] ? not x
;           [34:26] ? not x
            cmp_34_26_147_9:
            cmp r15b, 0
            jne if_34_23_147_9_end
            if_34_26_147_9_code:
;               [34:32] exit(1)
;               [34:32] allocate named register rdi
;               [34:37] 1
                mov rdi, 1
                mov rax, 60
                syscall
;               [34:32] free named register rdi
            if_34_23_147_9_end:
;           [147:9] free scratch register r15
        assert_147_9_end:
;   [150:5] assert(maybe == -1)
;   [150:12] allocate scratch register -> r15
;   [150:12] ? maybe == -1
;   [150:12] ? maybe == -1
    cmp_150_12:
;   [150:12] const eval to true
    bool_end_150_12:
    mov r15b, 1
;   [34:6] assert(x : bool)
    assert_150_5:
;       [150:5] alias x -> r15b
        if_34_26_150_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_150_5:
        cmp r15b, 0
        jne if_34_23_150_5_end
        if_34_26_150_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_150_5_end:
;       [150:5] free scratch register r15
    assert_150_5_end:
;   [152:5] var ix = 1
;   [152:9] ix: i64 (8 B @ [rbp + 248])
;   [152:9] ix = 1
;   [152:14] 1
    mov qword [rbp + 248], 1
;   [153:5] # variables can have an initial value that can be an expression
;   [155:5] arr[ix] = 2
;   [155:5] allocate scratch register -> r15
;   [155:9] set array index
;   [155:9] ix
    mov r15, qword [rbp + 248]
;   [155:9] bounds check
;   [155:9] allocate scratch register -> r14
;   [155:9] line number
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [155:9] free scratch register r14
;   [155:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [155:5] free scratch register r15
;   [156:5] arr[ix + 1] = arr[ix]
;   [156:5] allocate scratch register -> r15
;   [156:9] set array index
;   [156:9] ix
    mov r15, qword [rbp + 248]
;   [156:14] r15 + 1
    add r15, 1
;   [156:9] bounds check
;   [156:9] allocate scratch register -> r14
;   [156:9] line number
    mov r14, 156
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [156:9] free scratch register r14
;   [156:19] arr[ix]
;   [156:19] allocate scratch register -> r14
;   [156:23] set array index
;   [156:23] ix
    mov r14, qword [rbp + 248]
;   [156:23] bounds check
;   [156:23] allocate scratch register -> r13
;   [156:23] line number
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [156:23] free scratch register r13
;   [156:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [156:19] free scratch register r13
;   [156:19] free scratch register r14
;   [156:5] free scratch register r15
;   [157:5] assert(arr[1] == 2)
;   [157:12] allocate scratch register -> r15
;   [157:12] ? arr[1] == 2
;   [157:12] ? arr[1] == 2
    cmp_157_12:
;   [157:12] allocate scratch register -> r14
;   [157:16] set array index
;   [157:16] 1
    mov r14, 1
;   [157:16] bounds check
;   [157:16] allocate scratch register -> r13
;   [157:16] line number
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [157:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [157:12] free scratch register r14
    sete r15b
    bool_end_157_12:
;   [34:6] assert(x : bool)
    assert_157_5:
;       [157:5] alias x -> r15b
        if_34_26_157_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_157_5:
        cmp r15b, 0
        jne if_34_23_157_5_end
        if_34_26_157_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_157_5_end:
;       [157:5] free scratch register r15
    assert_157_5_end:
;   [158:5] assert(arr[2] == 2)
;   [158:12] allocate scratch register -> r15
;   [158:12] ? arr[2] == 2
;   [158:12] ? arr[2] == 2
    cmp_158_12:
;   [158:12] allocate scratch register -> r14
;   [158:16] set array index
;   [158:16] 2
    mov r14, 2
;   [158:16] bounds check
;   [158:16] allocate scratch register -> r13
;   [158:16] line number
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [158:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [158:12] free scratch register r14
    sete r15b
    bool_end_158_12:
;   [34:6] assert(x : bool)
    assert_158_5:
;       [158:5] alias x -> r15b
        if_34_26_158_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_158_5:
        cmp r15b, 0
        jne if_34_23_158_5_end
        if_34_26_158_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_158_5_end:
;       [158:5] free scratch register r15
    assert_158_5_end:
;   [160:5] array_copy(arr[2], arr, 2)
;   [160:5] allocate named register rsi
;   [160:5] allocate named register rdi
;   [160:5] allocate named register rcx
;   [160:29] 2
;   [160:29] 2
    mov rcx, 2
;   [160:16] arr[2]
;   [160:16] allocate scratch register -> r15
;   [160:20] set array index
;   [160:20] 2
    mov r15, 2
;   [160:20] bounds check
;   [160:20] allocate scratch register -> r14
;   [160:20] line number
    mov r14, 160
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [160:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [160:20] free scratch register r13
    cmovg rbp, r14
    jg baz_bounds_panic
;   [160:20] free scratch register r14
    lea rsi, [rbp + r15 * 4 + 224]
;   [160:5] free scratch register r15
;   [160:24] arr
;   [160:24] bounds check
;   [160:24] allocate scratch register -> r15
;   [160:24] line number
    mov r15, 160
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [160:24] free scratch register r15
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
;   [160:5] free named register rcx
;   [160:5] free named register rdi
;   [160:5] free named register rsi
;   [161:5] # copy from, to, number of elements
;   [162:5] assert(arr[0] == 2)
;   [162:12] allocate scratch register -> r15
;   [162:12] ? arr[0] == 2
;   [162:12] ? arr[0] == 2
    cmp_162_12:
;   [162:12] allocate scratch register -> r14
;   [162:16] set array index
;   [162:16] 0
    mov r14, 0
;   [162:16] bounds check
;   [162:16] allocate scratch register -> r13
;   [162:16] line number
    mov r13, 162
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [162:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [162:12] free scratch register r14
    sete r15b
    bool_end_162_12:
;   [34:6] assert(x : bool)
    assert_162_5:
;       [162:5] alias x -> r15b
        if_34_26_162_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_162_5:
        cmp r15b, 0
        jne if_34_23_162_5_end
        if_34_26_162_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_162_5_end:
;       [162:5] free scratch register r15
    assert_162_5_end:
;   [164:5] var arr1 : i32[8]
;   [164:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [164:9] zero 8 * 4 B = 32 B
;   [164:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [165:5] array_copy(arr, arr1, 4)
;   [165:5] allocate named register rsi
;   [165:5] allocate named register rdi
;   [165:5] allocate named register rcx
;   [165:27] 4
;   [165:27] 4
    mov rcx, 4
;   [165:16] arr
;   [165:16] bounds check
;   [165:16] allocate scratch register -> r15
;   [165:16] line number
    mov r15, 165
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [165:16] free scratch register r15
    lea rsi, [rbp + 224]
;   [165:21] arr1
;   [165:21] bounds check
;   [165:21] allocate scratch register -> r15
;   [165:21] line number
    mov r15, 165
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [165:21] free scratch register r15
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
;   [165:5] free named register rcx
;   [165:5] free named register rdi
;   [165:5] free named register rsi
;   [166:5] assert(arrays_equal(arr, arr1, 4))
;   [166:12] allocate scratch register -> r15
;   [166:12] ? arrays_equal(arr, arr1, 4)
;   [166:12] ? arrays_equal(arr, arr1, 4)
    cmp_166_12:
;   [166:12] allocate scratch register -> r14
;       [166:12] r14b = arrays_equal(arr, arr1, 4)
;       [166:12] = expression
;       [166:12] arrays_equal(arr, arr1, 4)
;       [166:12] allocate named register rsi
;       [166:12] allocate named register rdi
;       [166:12] allocate named register rcx
;       [166:36] 4
;       [166:36] 4
        mov rcx, 4
;       [166:25] arr
;       [166:25] bounds check
;       [166:25] allocate scratch register -> r13
;       [166:25] line number
        mov r13, 166
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [166:25] free scratch register r13
        lea rsi, [rbp + 224]
;       [166:30] arr1
;       [166:30] bounds check
;       [166:30] allocate scratch register -> r13
;       [166:30] line number
        mov r13, 166
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [166:30] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [166:12] free named register rcx
;       [166:12] free named register rdi
;       [166:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [166:12] free scratch register r14
    setne r15b
    bool_end_166_12:
;   [34:6] assert(x : bool)
    assert_166_5:
;       [166:5] alias x -> r15b
        if_34_26_166_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_166_5:
        cmp r15b, 0
        jne if_34_23_166_5_end
        if_34_26_166_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_166_5_end:
;       [166:5] free scratch register r15
    assert_166_5_end:
;   [167:5] # `arrays_equal` is built-in function
;   [169:5] arr1[2] = -1
;   [169:5] allocate scratch register -> r15
;   [169:10] set array index
;   [169:10] 2
    mov r15, 2
;   [169:10] bounds check
;   [169:10] allocate scratch register -> r14
;   [169:10] line number
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [169:10] free scratch register r14
;   [169:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [169:5] free scratch register r15
;   [170:5] assert(not arrays_equal(arr, arr1, 4))
;   [170:12] allocate scratch register -> r15
;   [170:12] ? not arrays_equal(arr, arr1, 4)
;   [170:12] ? not arrays_equal(arr, arr1, 4)
    cmp_170_12:
;   [170:16] allocate scratch register -> r14
;       [170:16] r14b = arrays_equal(arr, arr1, 4)
;       [170:16] = expression
;       [170:16] arrays_equal(arr, arr1, 4)
;       [170:16] allocate named register rsi
;       [170:16] allocate named register rdi
;       [170:16] allocate named register rcx
;       [170:40] 4
;       [170:40] 4
        mov rcx, 4
;       [170:29] arr
;       [170:29] bounds check
;       [170:29] allocate scratch register -> r13
;       [170:29] line number
        mov r13, 170
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [170:29] free scratch register r13
        lea rsi, [rbp + 224]
;       [170:34] arr1
;       [170:34] bounds check
;       [170:34] allocate scratch register -> r13
;       [170:34] line number
        mov r13, 170
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [170:34] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [170:16] free named register rcx
;       [170:16] free named register rdi
;       [170:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [170:12] free scratch register r14
    sete r15b
    bool_end_170_12:
;   [34:6] assert(x : bool)
    assert_170_5:
;       [170:5] alias x -> r15b
        if_34_26_170_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_170_5:
        cmp r15b, 0
        jne if_34_23_170_5_end
        if_34_26_170_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_170_5_end:
;       [170:5] free scratch register r15
    assert_170_5_end:
;   [172:5] ix = 3
;   [172:10] 3
    mov qword [rbp + 248], 3
;   [173:5] arr[ix] = ~inv(arr[ix - 1])
;   [173:5] allocate scratch register -> r15
;   [173:9] set array index
;   [173:9] ix
    mov r15, qword [rbp + 248]
;   [173:9] bounds check
;   [173:9] allocate scratch register -> r14
;   [173:9] line number
    mov r14, 173
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [173:9] free scratch register r14
;   [173:16] arr = ~inv(arr[ix - 1])
;   [173:16] = expression
;   [173:16] ~inv(arr[ix - 1])
;   [173:20] allocate scratch register -> r14
;   [173:24] set array index
;   [173:24] ix
    mov r14, qword [rbp + 248]
;   [173:29] r14 - 1
    sub r14, 1
;   [173:24] bounds check
;   [173:24] allocate scratch register -> r13
;   [173:24] line number
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [173:24] free scratch register r13
;   [58:6] inv(i : i32) : res i32
    inv_173_16:
;       [173:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [173:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [59:5] res = ~i
;       [59:12] ~i
;       [59:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [59:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [173:16] free scratch register r14
    inv_173_16_end:
    not dword [rbp + r15 * 4 + 224]
;   [173:5] free scratch register r15
;   [174:5] assert(arr[ix] == 2)
;   [174:12] allocate scratch register -> r15
;   [174:12] ? arr[ix] == 2
;   [174:12] ? arr[ix] == 2
    cmp_174_12:
;   [174:12] allocate scratch register -> r14
;   [174:16] set array index
;   [174:16] ix
    mov r14, qword [rbp + 248]
;   [174:16] bounds check
;   [174:16] allocate scratch register -> r13
;   [174:16] line number
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [174:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [174:12] free scratch register r14
    sete r15b
    bool_end_174_12:
;   [34:6] assert(x : bool)
    assert_174_5:
;       [174:5] alias x -> r15b
        if_34_26_174_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_174_5:
        cmp r15b, 0
        jne if_34_23_174_5_end
        if_34_26_174_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_174_5_end:
;       [174:5] free scratch register r15
    assert_174_5_end:
;   [176:5] faz(arr)
;   [68:6] faz(arg : i32[])
    faz_176_5:
;       [176:5] alias arg -> arr
;       [69:5] arg[1] = 0xfe
;       [69:5] allocate scratch register -> r15
;       [69:9] set array index
;       [69:9] 1
        mov r15, 1
;       [69:9] bounds check
;       [69:9] allocate scratch register -> r14
;       [69:9] line number
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
;       [69:9] free scratch register r14
;       [69:14] 0xfe
        mov dword [rbp + r15 * 4 + 224], 254
;       [69:5] free scratch register r15
    faz_176_5_end:
;   [177:5] assert(arr[1] == 0xfe)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? arr[1] == 0xfe
;   [177:12] ? arr[1] == 0xfe
    cmp_177_12:
;   [177:12] allocate scratch register -> r14
;   [177:16] set array index
;   [177:16] 1
    mov r14, 1
;   [177:16] bounds check
;   [177:16] allocate scratch register -> r13
;   [177:16] line number
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [177:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 254
;   [177:12] free scratch register r14
    sete r15b
    bool_end_177_12:
;   [34:6] assert(x : bool)
    assert_177_5:
;       [177:5] alias x -> r15b
        if_34_26_177_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_177_5:
        cmp r15b, 0
        jne if_34_23_177_5_end
        if_34_26_177_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_177_5_end:
;       [177:5] free scratch register r15
    assert_177_5_end:
;   [179:5] var arr3 : [] = { 3, 5 }
;   [179:9] arr3: i64[2] (16 B @ [rbp + 288])
;   [179:9] arr3 = { 3, 5 }
;   [179:23] [0]
;   [179:23] 3
    mov qword [rbp + 288], 3
;   [179:23] [1]
;   [179:26] 5
    mov qword [rbp + 296], 5
;   [180:5] foo arr3
;   [180:9] allocate scratch register -> r15
;   [180:9] e: i64 (r15)
;   [180:9] i: i64 (8 B @ [rbp + 312])
;   [180:9] const n = 2
;   [180:9] initiate iterator e
    lea r15, [rbp + 288]
;   [180:9] initiate counter i
    mov qword [rbp + 312], 0
    foo_180_5:
;       [181:9] e = e + i + n
;       [181:13] instructions without scratch register 3, with 4
;       [181:13] e
;       [181:17] e + i
;       [181:17] allocate scratch register -> r14
        mov r14, qword [rbp + 312]
        add qword [r15], r14
;       [181:17] free scratch register r14
;       [181:21] e + n
        add qword [r15], 2
        foo_180_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_180_5
    foo_180_5_end:
;   [180:5] free scratch register r15
;   [183:5] assert(arr3[0] == 3 + 0 + 2)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? arr3[0] == 3 + 0 + 2
;   [183:12] ? arr3[0] == 3 + 0 + 2
    cmp_183_12:
;   [183:12] allocate scratch register -> r14
;   [183:17] set array index
;   [183:17] 0
    mov r14, 0
;   [183:17] bounds check
;   [183:17] allocate scratch register -> r13
;   [183:17] line number
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [183:17] free scratch register r13
;   [183:23] allocate scratch register -> r13
;       [183:23] 3
        mov r13, 3
;       [183:27] r13 + 0
        add r13, 0
;       [183:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [183:12] free scratch register r13
;   [183:12] free scratch register r14
    sete r15b
    bool_end_183_12:
;   [34:6] assert(x : bool)
    assert_183_5:
;       [183:5] alias x -> r15b
        if_34_26_183_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_183_5:
        cmp r15b, 0
        jne if_34_23_183_5_end
        if_34_26_183_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_183_5_end:
;       [183:5] free scratch register r15
    assert_183_5_end:
;   [184:5] assert(arr3[1] == 5 + 1 + 2)
;   [184:12] allocate scratch register -> r15
;   [184:12] ? arr3[1] == 5 + 1 + 2
;   [184:12] ? arr3[1] == 5 + 1 + 2
    cmp_184_12:
;   [184:12] allocate scratch register -> r14
;   [184:17] set array index
;   [184:17] 1
    mov r14, 1
;   [184:17] bounds check
;   [184:17] allocate scratch register -> r13
;   [184:17] line number
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [184:17] free scratch register r13
;   [184:23] allocate scratch register -> r13
;       [184:23] 5
        mov r13, 5
;       [184:27] r13 + 1
        add r13, 1
;       [184:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [184:12] free scratch register r13
;   [184:12] free scratch register r14
    sete r15b
    bool_end_184_12:
;   [34:6] assert(x : bool)
    assert_184_5:
;       [184:5] alias x -> r15b
        if_34_26_184_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_184_5:
        cmp r15b, 0
        jne if_34_23_184_5_end
        if_34_26_184_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_184_5_end:
;       [184:5] free scratch register r15
    assert_184_5_end:
;   [185:5] # `foo` is a language construct that iterates over an array injecting:
;   [186:5] # `e`: current element
;   [187:5] # `i`: index starting at 0
;   [188:5] # `n`: constant array size
;   [190:5] var p : point = {0, 0}
;   [190:9] p: point (16 B @ [rbp + 304])
;   [190:9] p = {0, 0}
;   [190:22] copy field 'x'
    mov qword [rbp + 304], 0
;   [190:25] copy field 'y'
    mov qword [rbp + 312], 0
;   [191:5] fooz(p)
;   [42:6] fooz(pt : point)
    fooz_191_5:
;       [191:5] alias pt -> p
;       [43:5] pt.x = 0b10
;       [43:12] 0b10
        mov qword [rbp + 304], 2
;       [43:20] # binary value 2
;       [44:5] pt.y = 0xb
;       [44:12] 0xb
        mov qword [rbp + 312], 11
;       [44:20] # hex value 11
    fooz_191_5_end:
;   [192:5] assert(p.x == 2)
;   [192:12] allocate scratch register -> r15
;   [192:12] ? p.x == 2
;   [192:12] ? p.x == 2
    cmp_192_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_192_12:
;   [34:6] assert(x : bool)
    assert_192_5:
;       [192:5] alias x -> r15b
        if_34_26_192_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_192_5:
        cmp r15b, 0
        jne if_34_23_192_5_end
        if_34_26_192_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_192_5_end:
;       [192:5] free scratch register r15
    assert_192_5_end:
;   [193:5] assert(p.y == 0xb)
;   [193:12] allocate scratch register -> r15
;   [193:12] ? p.y == 0xb
;   [193:12] ? p.y == 0xb
    cmp_193_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_193_12:
;   [34:6] assert(x : bool)
    assert_193_5:
;       [193:5] alias x -> r15b
        if_34_26_193_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_193_5:
        cmp r15b, 0
        jne if_34_23_193_5_end
        if_34_26_193_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_193_5_end:
;       [193:5] free scratch register r15
    assert_193_5_end:
;   [195:5] var q : point = p
;   [195:9] q: point (16 B @ [rbp + 320])
;   [195:9] q = p
;   [195:21] size <= 16 B, use mov
;   [195:21] allocate named register rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
;   [195:21] free named register rax
;   [196:5] assert(equal(p, q))
;   [196:12] allocate scratch register -> r15
;   [196:12] ? equal(p, q)
;   [196:12] ? equal(p, q)
    cmp_196_12:
;   [196:12] allocate scratch register -> r14
;       [196:12] r14b = equal(p, q)
;       [196:12] = expression
;       [196:12] equal(p, q)
;       [196:12] allocate named register rsi
;       [196:12] allocate named register rdi
;       [196:12] allocate named register rcx
;       [196:18] p
        lea rsi, [rbp + 304]
;       [196:21] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [196:12] free named register rcx
;       [196:12] free named register rdi
;       [196:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [196:12] free scratch register r14
    setne r15b
    bool_end_196_12:
;   [34:6] assert(x : bool)
    assert_196_5:
;       [196:5] alias x -> r15b
        if_34_26_196_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_196_5:
        cmp r15b, 0
        jne if_34_23_196_5_end
        if_34_26_196_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_196_5_end:
;       [196:5] free scratch register r15
    assert_196_5_end:
;   [197:5] # `equal` is built-in function to compare user types for equality or same
;   [198:5] # size arrays
;   [200:5] q.x = 3
;   [200:11] 3
    mov qword [rbp + 320], 3
;   [201:5] assert(not equal(p, q))
;   [201:12] allocate scratch register -> r15
;   [201:12] ? not equal(p, q)
;   [201:12] ? not equal(p, q)
    cmp_201_12:
;   [201:16] allocate scratch register -> r14
;       [201:16] r14b = equal(p, q)
;       [201:16] = expression
;       [201:16] equal(p, q)
;       [201:16] allocate named register rsi
;       [201:16] allocate named register rdi
;       [201:16] allocate named register rcx
;       [201:22] p
        lea rsi, [rbp + 304]
;       [201:25] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [201:16] free named register rcx
;       [201:16] free named register rdi
;       [201:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [201:12] free scratch register r14
    sete r15b
    bool_end_201_12:
;   [34:6] assert(x : bool)
    assert_201_5:
;       [201:5] alias x -> r15b
        if_34_26_201_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_201_5:
        cmp r15b, 0
        jne if_34_23_201_5_end
        if_34_26_201_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_201_5_end:
;       [201:5] free scratch register r15
    assert_201_5_end:
;   [203:5] var i = 0
;   [203:9] i: i64 (8 B @ [rbp + 336])
;   [203:9] i = 0
;   [203:13] 0
    mov qword [rbp + 336], 0
;   [204:5] bar(i)
;   [49:6] bar(arg)
    bar_204_5:
;       [204:5] alias arg -> i
        if_50_8_204_5:
;       [50:8] ? arg == 0
;       [50:8] ? arg == 0
        cmp_50_8_204_5:
        cmp qword [rbp + 336], 0
        jne if_50_5_204_5_end
        if_50_8_204_5_code:
;           [50:17] return
            jmp bar_204_5_end
        if_50_5_204_5_end:
;       [51:5] arg = 0xff
;       [51:11] 0xff
        mov qword [rbp + 336], 255
    bar_204_5_end:
;   [205:5] assert(i == 0)
;   [205:12] allocate scratch register -> r15
;   [205:12] ? i == 0
;   [205:12] ? i == 0
    cmp_205_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_205_12:
;   [34:6] assert(x : bool)
    assert_205_5:
;       [205:5] alias x -> r15b
        if_34_26_205_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_205_5:
        cmp r15b, 0
        jne if_34_23_205_5_end
        if_34_26_205_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_205_5_end:
;       [205:5] free scratch register r15
    assert_205_5_end:
;   [207:5] i = 1
;   [207:9] 1
    mov qword [rbp + 336], 1
;   [208:5] bar(i)
;   [49:6] bar(arg)
    bar_208_5:
;       [208:5] alias arg -> i
        if_50_8_208_5:
;       [50:8] ? arg == 0
;       [50:8] ? arg == 0
        cmp_50_8_208_5:
        cmp qword [rbp + 336], 0
        jne if_50_5_208_5_end
        if_50_8_208_5_code:
;           [50:17] return
            jmp bar_208_5_end
        if_50_5_208_5_end:
;       [51:5] arg = 0xff
;       [51:11] 0xff
        mov qword [rbp + 336], 255
    bar_208_5_end:
;   [209:5] assert(i == 0xff)
;   [209:12] allocate scratch register -> r15
;   [209:12] ? i == 0xff
;   [209:12] ? i == 0xff
    cmp_209_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_209_12:
;   [34:6] assert(x : bool)
    assert_209_5:
;       [209:5] alias x -> r15b
        if_34_26_209_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_209_5:
        cmp r15b, 0
        jne if_34_23_209_5_end
        if_34_26_209_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_209_5_end:
;       [209:5] free scratch register r15
    assert_209_5_end:
;   [211:5] var j = 1
;   [211:9] j: i64 (8 B @ [rbp + 344])
;   [211:9] j = 1
;   [211:13] 1
    mov qword [rbp + 344], 1
;   [212:5] var k = baz(j)
;   [212:9] k: i64 (8 B @ [rbp + 352])
;   [212:9] k = baz(j)
;   [212:13] k = baz(j)
;   [212:13] = expression
;   [212:13] baz(j)
;   [62:6] baz(arg) : res
    baz_212_13:
;       [212:13] alias res -> k
;       [212:13] alias arg -> j
;       [63:5] res = arg * 2
;       [63:11] instructions without scratch register 5, with 3
;       [63:11] allocate scratch register -> r15
;       [63:11] arg
        mov r15, qword [rbp + 344]
;       [63:17] r15 * 2
;       [63:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [63:11] free scratch register r15
    baz_212_13_end:
;   [213:5] assert(k == 2)
;   [213:12] allocate scratch register -> r15
;   [213:12] ? k == 2
;   [213:12] ? k == 2
    cmp_213_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_213_12:
;   [34:6] assert(x : bool)
    assert_213_5:
;       [213:5] alias x -> r15b
        if_34_26_213_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_213_5:
        cmp r15b, 0
        jne if_34_23_213_5_end
        if_34_26_213_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_213_5_end:
;       [213:5] free scratch register r15
    assert_213_5_end:
;   [215:5] k = baz(1)
;   [215:9] k = baz(1)
;   [215:9] = expression
;   [215:9] baz(1)
;   [62:6] baz(arg) : res
    baz_215_9:
;       [215:9] alias res -> k
;       [215:9] alias arg -> 1
;       [63:5] res = arg * 2
;       [63:11] instructions without scratch register 4, with 3
;       [63:11] allocate scratch register -> r15
;       [63:11] arg
        mov r15, 1
;       [63:17] r15 * 2
;       [63:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [63:11] free scratch register r15
    baz_215_9_end:
;   [216:5] assert(k == 2)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? k == 2
;   [216:12] ? k == 2
    cmp_216_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_216_12:
;   [34:6] assert(x : bool)
    assert_216_5:
;       [216:5] alias x -> r15b
        if_34_26_216_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_216_5:
        cmp r15b, 0
        jne if_34_23_216_5_end
        if_34_26_216_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_216_5_end:
;       [216:5] free scratch register r15
    assert_216_5_end:
;   [218:5] var p0 : point = {baz(3), 0}
;   [218:9] p0: point (16 B @ [rbp + 360])
;   [218:9] p0 = {baz(3), 0}
;   [218:23] copy field 'x'
;   [218:23] p0.x = baz(3)
;   [218:23] = expression
;   [218:23] baz(3)
;   [62:6] baz(arg) : res
    baz_218_23:
;       [218:23] alias res -> p0.x (lea: rbp + 360)
;       [218:23] alias arg -> 3
;       [63:5] res = arg * 2
;       [63:11] instructions without scratch register 4, with 3
;       [63:11] allocate scratch register -> r15
;       [63:11] arg
        mov r15, 3
;       [63:17] r15 * 2
;       [63:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 360], r15
;       [63:11] free scratch register r15
    baz_218_23_end:
;   [218:31] copy field 'y'
    mov qword [rbp + 368], 0
;   [219:5] assert(p0.x == 6)
;   [219:12] allocate scratch register -> r15
;   [219:12] ? p0.x == 6
;   [219:12] ? p0.x == 6
    cmp_219_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_219_12:
;   [34:6] assert(x : bool)
    assert_219_5:
;       [219:5] alias x -> r15b
        if_34_26_219_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_219_5:
        cmp r15b, 0
        jne if_34_23_219_5_end
        if_34_26_219_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_219_5_end:
;       [219:5] free scratch register r15
    assert_219_5_end:
;   [221:5] var pt : point = point_init()
;   [221:9] pt: point (16 B @ [rbp + 376])
;   [221:9] pt = point_init()
;   [221:22] point_init()
;   [81:6] point_init() : res point
    point_init_221_22:
;       [221:22] alias res -> pt
;       [82:5] res.x = -1
;       [82:14] -1
        mov qword [rbp + 376], -1
;       [83:5] res.y = -2
;       [83:14] -2
        mov qword [rbp + 384], -2
    point_init_221_22_end:
;   [222:5] assert(pt.x == -1)
;   [222:12] allocate scratch register -> r15
;   [222:12] ? pt.x == -1
;   [222:12] ? pt.x == -1
    cmp_222_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_222_12:
;   [34:6] assert(x : bool)
    assert_222_5:
;       [222:5] alias x -> r15b
        if_34_26_222_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_222_5:
        cmp r15b, 0
        jne if_34_23_222_5_end
        if_34_26_222_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_222_5_end:
;       [222:5] free scratch register r15
    assert_222_5_end:
;   [223:5] assert(pt.y == -2)
;   [223:12] allocate scratch register -> r15
;   [223:12] ? pt.y == -2
;   [223:12] ? pt.y == -2
    cmp_223_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_223_12:
;   [34:6] assert(x : bool)
    assert_223_5:
;       [223:5] alias x -> r15b
        if_34_26_223_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_223_5:
        cmp r15b, 0
        jne if_34_23_223_5_end
        if_34_26_223_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_223_5_end:
;       [223:5] free scratch register r15
    assert_223_5_end:
;   [225:5] var x = 1
;   [225:9] x: i64 (8 B @ [rbp + 392])
;   [225:9] x = 1
;   [225:13] 1
    mov qword [rbp + 392], 1
;   [226:5] var y = 2
;   [226:9] y: i64 (8 B @ [rbp + 400])
;   [226:9] y = 2
;   [226:13] 2
    mov qword [rbp + 400], 2
;   [228:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [228:9] o1: object (20 B @ [rbp + 408])
;   [228:9] o1 = {{x * 10, y}, 0xff0000}
;   [228:24] copy field 'pos'
;   [228:25] copy field 'x'
;   [228:25] instructions without scratch register 5, with 3
;   [228:25] allocate scratch register -> r15
;   [228:25] x
    mov r15, qword [rbp + 392]
;   [228:29] r15 * 10
;   [228:29] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 408], r15
;   [228:25] free scratch register r15
;   [228:33] copy field 'y'
;   [228:33] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
;   [228:33] free scratch register r15
;   [228:37] copy field 'color'
    mov dword [rbp + 424], 16711680
;   [229:5] assert(o1.pos.x == 10)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? o1.pos.x == 10
;   [229:12] ? o1.pos.x == 10
    cmp_229_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_229_12:
;   [34:6] assert(x : bool)
    assert_229_5:
;       [229:5] alias x -> r15b
        if_34_26_229_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_229_5:
        cmp r15b, 0
        jne if_34_23_229_5_end
        if_34_26_229_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_229_5_end:
;       [229:5] free scratch register r15
    assert_229_5_end:
;   [230:5] assert(o1.pos.y == 2)
;   [230:12] allocate scratch register -> r15
;   [230:12] ? o1.pos.y == 2
;   [230:12] ? o1.pos.y == 2
    cmp_230_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_230_12:
;   [34:6] assert(x : bool)
    assert_230_5:
;       [230:5] alias x -> r15b
        if_34_26_230_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_230_5:
        cmp r15b, 0
        jne if_34_23_230_5_end
        if_34_26_230_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_230_5_end:
;       [230:5] free scratch register r15
    assert_230_5_end:
;   [231:5] assert(o1.color == 0xff0000)
;   [231:12] allocate scratch register -> r15
;   [231:12] ? o1.color == 0xff0000
;   [231:12] ? o1.color == 0xff0000
    cmp_231_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_231_12:
;   [34:6] assert(x : bool)
    assert_231_5:
;       [231:5] alias x -> r15b
        if_34_26_231_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_231_5:
        cmp r15b, 0
        jne if_34_23_231_5_end
        if_34_26_231_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_231_5_end:
;       [231:5] free scratch register r15
    assert_231_5_end:
;   [233:5] var p1 : point = {-x, -y}
;   [233:9] p1: point (16 B @ [rbp + 428])
;   [233:9] p1 = {-x, -y}
;   [233:23] copy field 'x'
;   [233:23] allocate scratch register -> r15
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
;   [233:23] free scratch register r15
    neg qword [rbp + 428]
;   [233:27] copy field 'y'
;   [233:27] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
;   [233:27] free scratch register r15
    neg qword [rbp + 436]
;   [234:5] o1.pos = p1
;   [234:14] size <= 16 B, use mov
;   [234:14] allocate named register rax
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
;   [234:14] free named register rax
;   [235:5] assert(o1.pos.x == -1)
;   [235:12] allocate scratch register -> r15
;   [235:12] ? o1.pos.x == -1
;   [235:12] ? o1.pos.x == -1
    cmp_235_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_235_12:
;   [34:6] assert(x : bool)
    assert_235_5:
;       [235:5] alias x -> r15b
        if_34_26_235_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_235_5:
        cmp r15b, 0
        jne if_34_23_235_5_end
        if_34_26_235_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_235_5_end:
;       [235:5] free scratch register r15
    assert_235_5_end:
;   [236:5] assert(o1.pos.y == -2)
;   [236:12] allocate scratch register -> r15
;   [236:12] ? o1.pos.y == -2
;   [236:12] ? o1.pos.y == -2
    cmp_236_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_236_12:
;   [34:6] assert(x : bool)
    assert_236_5:
;       [236:5] alias x -> r15b
        if_34_26_236_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_236_5:
        cmp r15b, 0
        jne if_34_23_236_5_end
        if_34_26_236_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_236_5_end:
;       [236:5] free scratch register r15
    assert_236_5_end:
;   [238:5] var o2 : object = o1
;   [238:9] o2: object (20 B @ [rbp + 444])
;   [238:9] o2 = o1
;   [238:23] allocate named register rsi
;   [238:23] allocate named register rdi
;   [238:23] allocate named register rcx
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
;   [238:23] free named register rcx
;   [238:23] free named register rdi
;   [238:23] free named register rsi
;   [239:5] assert(o2.pos.x == -1)
;   [239:12] allocate scratch register -> r15
;   [239:12] ? o2.pos.x == -1
;   [239:12] ? o2.pos.x == -1
    cmp_239_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_239_12:
;   [34:6] assert(x : bool)
    assert_239_5:
;       [239:5] alias x -> r15b
        if_34_26_239_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_239_5:
        cmp r15b, 0
        jne if_34_23_239_5_end
        if_34_26_239_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_239_5_end:
;       [239:5] free scratch register r15
    assert_239_5_end:
;   [240:5] assert(o2.pos.y == -2)
;   [240:12] allocate scratch register -> r15
;   [240:12] ? o2.pos.y == -2
;   [240:12] ? o2.pos.y == -2
    cmp_240_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_240_12:
;   [34:6] assert(x : bool)
    assert_240_5:
;       [240:5] alias x -> r15b
        if_34_26_240_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_240_5:
        cmp r15b, 0
        jne if_34_23_240_5_end
        if_34_26_240_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_240_5_end:
;       [240:5] free scratch register r15
    assert_240_5_end:
;   [241:5] assert(o2.color == 0xff0000)
;   [241:12] allocate scratch register -> r15
;   [241:12] ? o2.color == 0xff0000
;   [241:12] ? o2.color == 0xff0000
    cmp_241_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_241_12:
;   [34:6] assert(x : bool)
    assert_241_5:
;       [241:5] alias x -> r15b
        if_34_26_241_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_241_5:
        cmp r15b, 0
        jne if_34_23_241_5_end
        if_34_26_241_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_241_5_end:
;       [241:5] free scratch register r15
    assert_241_5_end:
;   [243:5] var o3 : object[2]
;   [243:9] o3: object[2] (40 B @ [rbp + 464])
;   [243:9] zero 2 * 20 B = 40 B
;   [243:5] allocate named register rax
;   [243:5] allocate named register rdi
;   [243:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
;   [243:5] free named register rcx
;   [243:5] free named register rdi
;   [243:5] free named register rax
;   [244:5] o3.pos.y = 73
;   [244:16] 73
    mov qword [rbp + 472], 73
;   [245:5] # index 0 in an array can be accessed without array index
;   [247:5] assert(o3[0].pos.y == 73)
;   [247:12] allocate scratch register -> r15
;   [247:12] ? o3[0].pos.y == 73
;   [247:12] ? o3[0].pos.y == 73
    cmp_247_12:
;   [247:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [247:12] allocate scratch register -> r13
;   [247:15] set array index
;   [247:15] 0
    mov r13, 0
;   [247:15] bounds check
;   [247:15] allocate scratch register -> r12
;   [247:15] line number
    mov r12, 247
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [247:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [247:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [247:12] free scratch register r14
    sete r15b
    bool_end_247_12:
;   [34:6] assert(x : bool)
    assert_247_5:
;       [247:5] alias x -> r15b
        if_34_26_247_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_247_5:
        cmp r15b, 0
        jne if_34_23_247_5_end
        if_34_26_247_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_247_5_end:
;       [247:5] free scratch register r15
    assert_247_5_end:
;   [249:5] o3[1] = object_init()
;   [249:5] allocate scratch register -> r15
    lea r15, [rbp + 464]
;   [249:5] allocate scratch register -> r14
;   [249:8] set array index
;   [249:8] 1
    mov r14, 1
;   [249:8] bounds check
;   [249:8] allocate scratch register -> r13
;   [249:8] line number
    mov r13, 249
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [249:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [249:5] free scratch register r14
;   [249:13] object_init()
;   [86:6] object_init() : res object
    object_init_249_13:
;       [249:13] alias res -> o3 (lea: r15)
;       [87:5] res.pos.y = 74
;       [87:17] 74
        mov qword [r15 + 8], 74
    object_init_249_13_end:
;   [249:5] free scratch register r15
;   [250:5] assert(o3[1].pos.y == 74)
;   [250:12] allocate scratch register -> r15
;   [250:12] ? o3[1].pos.y == 74
;   [250:12] ? o3[1].pos.y == 74
    cmp_250_12:
;   [250:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [250:12] allocate scratch register -> r13
;   [250:15] set array index
;   [250:15] 1
    mov r13, 1
;   [250:15] bounds check
;   [250:15] allocate scratch register -> r12
;   [250:15] line number
    mov r12, 250
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [250:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [250:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [250:12] free scratch register r14
    sete r15b
    bool_end_250_12:
;   [34:6] assert(x : bool)
    assert_250_5:
;       [250:5] alias x -> r15b
        if_34_26_250_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_250_5:
        cmp r15b, 0
        jne if_34_23_250_5_end
        if_34_26_250_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_250_5_end:
;       [250:5] free scratch register r15
    assert_250_5_end:
;   [252:5] var worlds : world[8]
;   [252:9] worlds: world[8] (512 B @ [rbp + 504])
;   [252:9] zero 8 * 64 B = 512 B
;   [252:5] allocate named register rax
;   [252:5] allocate named register rdi
;   [252:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
;   [252:5] free named register rcx
;   [252:5] free named register rdi
;   [252:5] free named register rax
;   [253:5] worlds[1].locations[1] = 0xffee
;   [253:5] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [253:5] allocate scratch register -> r14
;   [253:12] set array index
;   [253:12] 1
    mov r14, 1
;   [253:12] bounds check
;   [253:12] allocate scratch register -> r13
;   [253:12] line number
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [253:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [253:5] free scratch register r14
;   [253:5] allocate scratch register -> r14
;   [253:25] set array index
;   [253:25] 1
    mov r14, 1
;   [253:25] bounds check
;   [253:25] allocate scratch register -> r13
;   [253:25] line number
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [253:25] free scratch register r13
;   [253:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [253:5] free scratch register r14
;   [253:5] free scratch register r15
;   [254:5] assert(worlds[1].locations[1] == 0xffee)
;   [254:12] allocate scratch register -> r15
;   [254:12] ? worlds[1].locations[1] == 0xffee
;   [254:12] ? worlds[1].locations[1] == 0xffee
    cmp_254_12:
;   [254:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [254:12] allocate scratch register -> r13
;   [254:19] set array index
;   [254:19] 1
    mov r13, 1
;   [254:19] bounds check
;   [254:19] allocate scratch register -> r12
;   [254:19] line number
    mov r12, 254
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [254:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [254:12] free scratch register r13
;   [254:12] allocate scratch register -> r13
;   [254:32] set array index
;   [254:32] 1
    mov r13, 1
;   [254:32] bounds check
;   [254:32] allocate scratch register -> r12
;   [254:32] line number
    mov r12, 254
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [254:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [254:12] free scratch register r13
;   [254:12] free scratch register r14
    sete r15b
    bool_end_254_12:
;   [34:6] assert(x : bool)
    assert_254_5:
;       [254:5] alias x -> r15b
        if_34_26_254_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_254_5:
        cmp r15b, 0
        jne if_34_23_254_5_end
        if_34_26_254_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_254_5_end:
;       [254:5] free scratch register r15
    assert_254_5_end:
;   [256:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [256:5] allocate named register rsi
;   [256:5] allocate named register rdi
;   [256:5] allocate named register rcx
;   [259:9] array_size_of(worlds.locations)
;   [259:9] rcx = array_size_of(worlds.locations)
;   [259:9] = expression
;   [259:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [257:9] worlds[1].locations
;   [257:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [257:9] allocate scratch register -> r14
;   [257:16] set array index
;   [257:16] 1
    mov r14, 1
;   [257:16] bounds check
;   [257:16] allocate scratch register -> r13
;   [257:16] line number
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
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
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [257:9] free scratch register r14
    lea rsi, [r15]
;   [256:5] free scratch register r15
;   [258:9] worlds[0].locations
;   [258:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [258:9] allocate scratch register -> r14
;   [258:16] set array index
;   [258:16] 0
    mov r14, 0
;   [258:16] bounds check
;   [258:16] allocate scratch register -> r13
;   [258:16] line number
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [258:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [258:9] free scratch register r14
;   [258:9] bounds check
;   [258:9] allocate scratch register -> r14
;   [258:9] line number
    mov r14, 258
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [258:9] free scratch register r14
    lea rdi, [r15]
;   [256:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [256:5] free named register rcx
;   [256:5] free named register rdi
;   [256:5] free named register rsi
;   [261:5] # `array_copy` is built-in and can use indexed positions
;   [262:5] # `array_size_of` is built-in
;   [264:5] assert(worlds[0].locations[1] == 0xffee)
;   [264:12] allocate scratch register -> r15
;   [264:12] ? worlds[0].locations[1] == 0xffee
;   [264:12] ? worlds[0].locations[1] == 0xffee
    cmp_264_12:
;   [264:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [264:12] allocate scratch register -> r13
;   [264:19] set array index
;   [264:19] 0
    mov r13, 0
;   [264:19] bounds check
;   [264:19] allocate scratch register -> r12
;   [264:19] line number
    mov r12, 264
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [264:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [264:12] free scratch register r13
;   [264:12] allocate scratch register -> r13
;   [264:32] set array index
;   [264:32] 1
    mov r13, 1
;   [264:32] bounds check
;   [264:32] allocate scratch register -> r12
;   [264:32] line number
    mov r12, 264
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [264:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [264:12] free scratch register r13
;   [264:12] free scratch register r14
    sete r15b
    bool_end_264_12:
;   [34:6] assert(x : bool)
    assert_264_5:
;       [264:5] alias x -> r15b
        if_34_26_264_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_264_5:
        cmp r15b, 0
        jne if_34_23_264_5_end
        if_34_26_264_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_264_5_end:
;       [264:5] free scratch register r15
    assert_264_5_end:
;   [265:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [265:12] allocate scratch register -> r15
;   [265:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [265:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_265_12:
;   [265:12] allocate scratch register -> r14
;       [265:12] r14b = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [265:12] = expression
;       [265:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [265:12] allocate named register rsi
;       [265:12] allocate named register rdi
;       [265:12] allocate named register rcx
;       [268:14] array_size_of(worlds.locations)
;       [268:14] rcx = array_size_of(worlds.locations)
;       [268:14] = expression
;       [268:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [266:14] worlds[0].locations
;       [266:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [266:14] allocate scratch register -> r12
;       [266:21] set array index
;       [266:21] 0
        mov r12, 0
;       [266:21] bounds check
;       [266:21] allocate scratch register -> r10
;       [266:21] line number
        mov r10, 266
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
;       [266:21] free scratch register r10
        shl r12, 6
        add r13, r12
;       [266:14] free scratch register r12
;       [266:14] bounds check
;       [266:14] allocate scratch register -> r12
;       [266:14] line number
        mov r12, 266
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [266:14] free scratch register r12
        lea rsi, [r13]
;       [265:12] free scratch register r13
;       [267:14] worlds[1].locations
;       [267:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [267:14] allocate scratch register -> r12
;       [267:21] set array index
;       [267:21] 1
        mov r12, 1
;       [267:21] bounds check
;       [267:21] allocate scratch register -> r10
;       [267:21] line number
        mov r10, 267
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
;       [267:21] free scratch register r10
        shl r12, 6
        add r13, r12
;       [267:14] free scratch register r12
;       [267:14] bounds check
;       [267:14] allocate scratch register -> r12
;       [267:14] line number
        mov r12, 267
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [267:14] free scratch register r12
        lea rdi, [r13]
;       [265:12] free scratch register r13
        shl rcx, 3
        repe cmpsb
;       [265:12] free named register rcx
;       [265:12] free named register rdi
;       [265:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [265:12] free scratch register r14
    setne r15b
    bool_end_265_12:
;   [34:6] assert(x : bool)
    assert_265_5:
;       [265:5] alias x -> r15b
        if_34_26_265_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_265_5:
        cmp r15b, 0
        jne if_34_23_265_5_end
        if_34_26_265_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_265_5_end:
;       [265:5] free scratch register r15
    assert_265_5_end:
;   [271:5] var arr2 : [] = { -1, 2 }
;   [271:9] arr2: i64[2] (16 B @ [rbp + 1016])
;   [271:9] arr2 = { -1, 2 }
;   [271:23] [0]
;   [271:24] -1
    mov qword [rbp + 1016], -1
;   [271:23] [1]
;   [271:27] 2
    mov qword [rbp + 1024], 2
;   [272:5] assert(array_size_of(arr2) == 2)
;   [272:12] allocate scratch register -> r15
;   [272:12] ? array_size_of(arr2) == 2
;   [272:12] ? array_size_of(arr2) == 2
    cmp_272_12:
;   [272:12] allocate scratch register -> r14
;       [272:12] r14 = array_size_of(arr2)
;       [272:12] = expression
;       [272:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [272:12] free scratch register r14
    sete r15b
    bool_end_272_12:
;   [34:6] assert(x : bool)
    assert_272_5:
;       [272:5] alias x -> r15b
        if_34_26_272_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_272_5:
        cmp r15b, 0
        jne if_34_23_272_5_end
        if_34_26_272_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_272_5_end:
;       [272:5] free scratch register r15
    assert_272_5_end:
;   [273:5] assert(arr2[0] == -1)
;   [273:12] allocate scratch register -> r15
;   [273:12] ? arr2[0] == -1
;   [273:12] ? arr2[0] == -1
    cmp_273_12:
;   [273:12] allocate scratch register -> r14
;   [273:17] set array index
;   [273:17] 0
    mov r14, 0
;   [273:17] bounds check
;   [273:17] allocate scratch register -> r13
;   [273:17] line number
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [273:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], -1
;   [273:12] free scratch register r14
    sete r15b
    bool_end_273_12:
;   [34:6] assert(x : bool)
    assert_273_5:
;       [273:5] alias x -> r15b
        if_34_26_273_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_273_5:
        cmp r15b, 0
        jne if_34_23_273_5_end
        if_34_26_273_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_273_5_end:
;       [273:5] free scratch register r15
    assert_273_5_end:
;   [274:5] assert(arr2[1] == 2)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? arr2[1] == 2
;   [274:12] ? arr2[1] == 2
    cmp_274_12:
;   [274:12] allocate scratch register -> r14
;   [274:17] set array index
;   [274:17] 1
    mov r14, 1
;   [274:17] bounds check
;   [274:17] allocate scratch register -> r13
;   [274:17] line number
    mov r13, 274
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [274:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], 2
;   [274:12] free scratch register r14
    sete r15b
    bool_end_274_12:
;   [34:6] assert(x : bool)
    assert_274_5:
;       [274:5] alias x -> r15b
        if_34_26_274_5:
;       [34:26] ? not x
;       [34:26] ? not x
        cmp_34_26_274_5:
        cmp r15b, 0
        jne if_34_23_274_5_end
        if_34_26_274_5_code:
;           [34:32] exit(1)
;           [34:32] allocate named register rdi
;           [34:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [34:32] free named register rdi
        if_34_23_274_5_end:
;       [274:5] free scratch register r15
    assert_274_5_end:
;   [276:5] var counter
;   [276:9] counter: i64 (8 B @ [rbp + 1032])
;   [276:9] zero 1 * 8 B = 8 B
;   [276:5] size <= 32 B, use mov
    mov qword [rbp + 1032], 0
;   [277:5] var nm : str
;   [277:9] nm: str (128 B @ [rbp + 1040])
;   [277:9] zero 1 * 128 B = 128 B
;   [277:5] allocate named register rax
;   [277:5] allocate named register rdi
;   [277:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
;   [277:5] free named register rcx
;   [277:5] free named register rdi
;   [277:5] free named register rax
;   [278:5] print(hello)
;   [36:6] print(str : i8[])
    print_278_5:
;       [278:5] alias str -> hello
;       [37:5] write(1, address_of(str), array_size_of(str))
;       [37:5] allocate named register rdi
;       [37:11] 1
        mov rdi, 1
;       [37:5] allocate named register rsi
;       [37:14] rsi = address_of(str)
;       [37:14] = expression
;       [37:14] address_of(str)
        lea rsi, [rbp]
;       [37:5] allocate named register rdx
;       [37:31] rdx = array_size_of(str)
;       [37:31] = expression
;       [37:31] array_size_of(str)
        mov rdx, 21
;       [37:5] allocate named register rax
        mov rax, 1
        syscall
;       [37:5] free named register rax
;       [37:5] free named register rdx
;       [37:5] free named register rsi
;       [37:5] free named register rdi
    print_278_5_end:
;   [279:5] label
    loop_279_5:
;       [280:9] counter = counter + 1
;       [280:19] instructions without scratch register 1, with 3
;       [280:19] counter
;       [280:29] counter + 1
        add qword [rbp + 1032], 1
;       [281:9] print_num(counter)
;       [281:9] address of argument 'counter' to parameter 'num'
;       [281:9] allocate scratch register -> r15
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
;       [281:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
;       [282:9] print(colon)
;       [36:6] print(str : i8[])
        print_282_9:
;           [282:9] alias str -> colon
;           [37:5] write(1, address_of(str), array_size_of(str))
;           [37:5] allocate named register rdi
;           [37:11] 1
            mov rdi, 1
;           [37:5] allocate named register rsi
;           [37:14] rsi = address_of(str)
;           [37:14] = expression
;           [37:14] address_of(str)
            lea rsi, [rbp + 61]
;           [37:5] allocate named register rdx
;           [37:31] rdx = array_size_of(str)
;           [37:31] = expression
;           [37:31] array_size_of(str)
            mov rdx, 2
;           [37:5] allocate named register rax
            mov rax, 1
            syscall
;           [37:5] free named register rax
;           [37:5] free named register rdx
;           [37:5] free named register rsi
;           [37:5] free named register rdi
        print_282_9_end:
;       [283:9] print(prompt1)
;       [36:6] print(str : i8[])
        print_283_9:
;           [283:9] alias str -> prompt1
;           [37:5] write(1, address_of(str), array_size_of(str))
;           [37:5] allocate named register rdi
;           [37:11] 1
            mov rdi, 1
;           [37:5] allocate named register rsi
;           [37:14] rsi = address_of(str)
;           [37:14] = expression
;           [37:14] address_of(str)
            lea rsi, [rbp + 21]
;           [37:5] allocate named register rdx
;           [37:31] rdx = array_size_of(str)
;           [37:31] = expression
;           [37:31] array_size_of(str)
            mov rdx, 12
;           [37:5] allocate named register rax
            mov rax, 1
            syscall
;           [37:5] free named register rax
;           [37:5] free named register rdx
;           [37:5] free named register rsi
;           [37:5] free named register rdi
        print_283_9_end:
;       [284:9] str_in(nm)
;       [72:6] str_in(s : str)
        str_in_284_9:
;           [284:9] alias s -> nm
;           [73:5] var nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [73:9] nbytes: i64 (8 B @ [rbp + 1168])
;           [73:9] nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [73:18] nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [73:18] = expression
;           [73:18] read(0, address_of(s.data), array_size_of(s.data))
;           [73:18] allocate named register rdi
;           [73:23] 0
            mov rdi, 0
;           [73:18] allocate named register rsi
;           [73:26] rsi = address_of(s.data)
;           [73:26] = expression
;           [73:26] address_of(s.data)
            lea rsi, [rbp + 1041]
;           [73:18] allocate named register rdx
;           [73:46] rdx = array_size_of(s.data)
;           [73:46] = expression
;           [73:46] array_size_of(s.data)
            mov rdx, 127
;           [73:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
;           [73:18] free named register rax
;           [73:18] free named register rdx
;           [73:18] free named register rsi
;           [73:18] free named register rdi
;           [74:5] s.len = nbytes - 1
;           [74:13] instructions without scratch register 3, with 3
;           [74:13] nbytes
;           [74:13] allocate scratch register -> r15
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
;           [74:13] free scratch register r15
;           [74:22] s.len - 1
            sub byte [rbp + 1040], 1
        str_in_284_9_end:
        if_285_12:
;       [285:12] ? nm.len == 0
;       [285:12] ? nm.len == 0
        cmp_285_12:
        cmp byte [rbp + 1040], 0
        jne if_287_19
        if_285_12_code:
;           [286:13] break
            jmp loop_279_5_end
        jmp if_285_9_end
        if_287_19:
;       [287:19] ? nm.len <= 4
;       [287:19] ? nm.len <= 4
        cmp_287_19:
        cmp byte [rbp + 1040], 4
        jg if_else_285_9
        if_287_19_code:
;           [288:13] print(prompt2)
;           [36:6] print(str : i8[])
            print_288_13:
;               [288:13] alias str -> prompt2
;               [37:5] write(1, address_of(str), array_size_of(str))
;               [37:5] allocate named register rdi
;               [37:11] 1
                mov rdi, 1
;               [37:5] allocate named register rsi
;               [37:14] rsi = address_of(str)
;               [37:14] = expression
;               [37:14] address_of(str)
                lea rsi, [rbp + 33]
;               [37:5] allocate named register rdx
;               [37:31] rdx = array_size_of(str)
;               [37:31] = expression
;               [37:31] array_size_of(str)
                mov rdx, 20
;               [37:5] allocate named register rax
                mov rax, 1
                syscall
;               [37:5] free named register rax
;               [37:5] free named register rdx
;               [37:5] free named register rsi
;               [37:5] free named register rdi
            print_288_13_end:
;           [289:13] continue
            jmp loop_279_5
        jmp if_285_9_end
        if_else_285_9:
;           [291:13] print(prompt3)
;           [36:6] print(str : i8[])
            print_291_13:
;               [291:13] alias str -> prompt3
;               [37:5] write(1, address_of(str), array_size_of(str))
;               [37:5] allocate named register rdi
;               [37:11] 1
                mov rdi, 1
;               [37:5] allocate named register rsi
;               [37:14] rsi = address_of(str)
;               [37:14] = expression
;               [37:14] address_of(str)
                lea rsi, [rbp + 53]
;               [37:5] allocate named register rdx
;               [37:31] rdx = array_size_of(str)
;               [37:31] = expression
;               [37:31] array_size_of(str)
                mov rdx, 6
;               [37:5] allocate named register rax
                mov rax, 1
                syscall
;               [37:5] free named register rax
;               [37:5] free named register rdx
;               [37:5] free named register rsi
;               [37:5] free named register rdi
            print_291_13_end:
;           [292:13] str_out(nm)
;           [77:6] str_out(s : str)
            str_out_292_13:
;               [292:13] alias s -> nm
;               [78:5] write(1, address_of(s.data), s.len)
;               [78:5] allocate named register rdi
;               [78:11] 1
                mov rdi, 1
;               [78:5] allocate named register rsi
;               [78:14] rsi = address_of(s.data)
;               [78:14] = expression
;               [78:14] address_of(s.data)
                lea rsi, [rbp + 1041]
;               [78:5] allocate named register rdx
;               [78:34] s.len
                movsx rdx, byte [rbp + 1040]
;               [78:5] allocate named register rax
                mov rax, 1
                syscall
;               [78:5] free named register rax
;               [78:5] free named register rdx
;               [78:5] free named register rsi
;               [78:5] free named register rdi
            str_out_292_13_end:
;           [293:13] print(dot)
;           [36:6] print(str : i8[])
            print_293_13:
;               [293:13] alias str -> dot
;               [37:5] write(1, address_of(str), array_size_of(str))
;               [37:5] allocate named register rdi
;               [37:11] 1
                mov rdi, 1
;               [37:5] allocate named register rsi
;               [37:14] rsi = address_of(str)
;               [37:14] = expression
;               [37:14] address_of(str)
                lea rsi, [rbp + 59]
;               [37:5] allocate named register rdx
;               [37:31] rdx = array_size_of(str)
;               [37:31] = expression
;               [37:31] array_size_of(str)
                mov rdx, 1
;               [37:5] allocate named register rax
                mov rax, 1
                syscall
;               [37:5] free named register rax
;               [37:5] free named register rdx
;               [37:5] free named register rsi
;               [37:5] free named register rdi
            print_293_13_end:
;           [294:13] print(nl)
;           [36:6] print(str : i8[])
            print_294_13:
;               [294:13] alias str -> nl
;               [37:5] write(1, address_of(str), array_size_of(str))
;               [37:5] allocate named register rdi
;               [37:11] 1
                mov rdi, 1
;               [37:5] allocate named register rsi
;               [37:14] rsi = address_of(str)
;               [37:14] = expression
;               [37:14] address_of(str)
                lea rsi, [rbp + 60]
;               [37:5] allocate named register rdx
;               [37:31] rdx = array_size_of(str)
;               [37:31] = expression
;               [37:31] array_size_of(str)
                mov rdx, 1
;               [37:5] allocate named register rax
                mov rax, 1
                syscall
;               [37:5] free named register rax
;               [37:5] free named register rdx
;               [37:5] free named register rsi
;               [37:5] free named register rdi
            print_294_13_end:
        if_285_9_end:
    jmp loop_279_5
    loop_279_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; 
;[98:15] noinline print_num(num)
print_num:
;   [98:25] num: i64 (8 B @ [rbx])
;   [99:5] var buf : i8[20]
;   [99:9] buf: i8[20] (20 B @ [rbx + 8])
;   [99:9] zero 20 * 1 B = 20 B
;   [99:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [100:5] var n = num
;   [100:9] n: i64 (8 B @ [rbx + 28])
;   [100:9] n = num
;   [100:13] num
;   [100:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [100:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
;   [100:13] free scratch register r14
;   [100:13] free scratch register r15
;   [101:5] var is_negative : bool = false
;   [101:9] is_negative: bool (1 B @ [rbx + 36])
;   [101:9] is_negative = false
    mov byte [rbx + 36], 0
    if_103_8:
;   [103:8] ? n < 0
;   [103:8] ? n < 0
    cmp_103_8:
    cmp qword [rbx + 28], 0
    jge if_103_5_end
    if_103_8_code:
;       [104:9] is_negative = true
        mov byte [rbx + 36], 1
;       [105:9] n = -n
;       [105:14] -n
        neg qword [rbx + 28]
    if_103_5_end:
;   [108:5] var i = 20
;   [108:9] i: i64 (8 B @ [rbx + 37])
;   [108:9] i = 20
;   [108:13] 20
    mov qword [rbx + 37], 20
;   [109:5] label
    loop_109_5:
;       [110:9] i = i - 1
;       [110:13] instructions without scratch register 1, with 3
;       [110:13] i
;       [110:17] i - 1
        sub qword [rbx + 37], 1
;       [111:9] var ascii = 48 + (n % 10)
;       [111:13] ascii: i64 (8 B @ [rbx + 45])
;       [111:13] ascii = 48 + (n % 10)
;       [111:21] instructions without scratch register 8, with 9
;       [111:21] 48
        mov qword [rbx + 45], 48
;       [111:27] ascii + (n % 10)
;       [111:27] allocate scratch register -> r15
;       [111:27] n
        mov r15, qword [rbx + 28]
;       [111:31] r15 % 10
;       [111:31] div const
;       [111:31] allocate named register rax
        mov rax, r15
;       [111:31] allocate named register rdx
        cqo
;       [111:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [111:31] free scratch register r14
        mov r15, rdx
;       [111:31] free named register rdx
;       [111:31] free named register rax
        add qword [rbx + 45], r15
;       [111:27] free scratch register r15
;       [112:9] # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
;       [113:9] buf[i] = ascii
;       [113:9] allocate scratch register -> r15
;       [113:13] set array index
;       [113:13] i
        mov r15, qword [rbx + 37]
;       [113:13] bounds check
;       [113:13] allocate scratch register -> r14
;       [113:13] line number
        mov r14, 113
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [113:13] free scratch register r14
;       [113:18] ascii
;       [113:18] allocate scratch register -> r14
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
;       [113:18] free scratch register r14
;       [113:9] free scratch register r15
;       [114:9] n = n / 10
;       [114:13] instructions without scratch register 5, with 7
;       [114:13] n
;       [114:17] n / 10
;       [114:17] div const
;       [114:17] allocate named register rax
        mov rax, qword [rbx + 28]
;       [114:17] allocate named register rdx
        cqo
;       [114:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [114:17] free scratch register r15
        mov qword [rbx + 28], rax
;       [114:17] free named register rdx
;       [114:17] free named register rax
        if_115_12:
;       [115:12] ? n == 0
;       [115:12] ? n == 0
        cmp_115_12:
        cmp qword [rbx + 28], 0
        jne if_115_9_end
        if_115_12_code:
;           [115:19] break
            jmp loop_109_5_end
        if_115_9_end:
    jmp loop_109_5
    loop_109_5_end:
    if_118_8:
;   [118:8] ? is_negative
;   [118:8] ? is_negative
    cmp_118_8:
    cmp byte [rbx + 36], 0
    je if_118_5_end
    if_118_8_code:
;       [119:9] i = i - 1
;       [119:13] instructions without scratch register 1, with 3
;       [119:13] i
;       [119:17] i - 1
        sub qword [rbx + 37], 1
;       [120:9] buf[i] = 45
;       [120:9] allocate scratch register -> r15
;       [120:13] set array index
;       [120:13] i
        mov r15, qword [rbx + 37]
;       [120:13] bounds check
;       [120:13] allocate scratch register -> r14
;       [120:13] line number
        mov r14, 120
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [120:13] free scratch register r14
;       [120:18] 45
        mov byte [rbx + r15 + 8], 45
;       [120:9] free scratch register r15
    if_118_5_end:
;   [123:5] var write_pos = 0
;   [123:9] write_pos: i64 (8 B @ [rbx + 45])
;   [123:9] write_pos = 0
;   [123:21] 0
    mov qword [rbx + 45], 0
;   [124:5] label
    loop_124_5:
;       [125:9] buf[write_pos] = buf[i]
;       [125:9] allocate scratch register -> r15
;       [125:13] set array index
;       [125:13] write_pos
        mov r15, qword [rbx + 45]
;       [125:13] bounds check
;       [125:13] allocate scratch register -> r14
;       [125:13] line number
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [125:13] free scratch register r14
;       [125:26] buf[i]
;       [125:26] allocate scratch register -> r14
;       [125:30] set array index
;       [125:30] i
        mov r14, qword [rbx + 37]
;       [125:30] bounds check
;       [125:30] allocate scratch register -> r13
;       [125:30] line number
        mov r13, 125
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [125:30] free scratch register r13
;       [125:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [125:26] free scratch register r13
;       [125:26] free scratch register r14
;       [125:9] free scratch register r15
;       [126:9] write_pos = write_pos + 1
;       [126:21] instructions without scratch register 1, with 3
;       [126:21] write_pos
;       [126:33] write_pos + 1
        add qword [rbx + 45], 1
;       [127:9] i = i + 1
;       [127:13] instructions without scratch register 1, with 3
;       [127:13] i
;       [127:17] i + 1
        add qword [rbx + 37], 1
        if_128_12:
;       [128:12] ? i == 20
;       [128:12] ? i == 20
        cmp_128_12:
        cmp qword [rbx + 37], 20
        jne if_128_9_end
        if_128_12_code:
;           [128:20] break
            jmp loop_124_5_end
        if_128_9_end:
    jmp loop_124_5
    loop_124_5_end:
;   [131:5] write(1, address_of(buf), write_pos)
;   [131:5] allocate named register rdi
;   [131:11] 1
    mov rdi, 1
;   [131:5] allocate named register rsi
;   [131:14] rsi = address_of(buf)
;   [131:14] = expression
;   [131:14] address_of(buf)
    lea rsi, [rbx + 8]
;   [131:5] allocate named register rdx
;   [131:31] write_pos
    mov rdx, qword [rbx + 45]
;   [131:5] allocate named register rax
    mov rax, 1
    syscall
;   [131:5] free named register rax
;   [131:5] free named register rdx
;   [131:5] free named register rsi
;   [131:5] free named register rdi
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
; i64[4]
;[27:23] [0]
;[27:23] i64
dq 1
; pad 3 'i64' of size 8
times 24 db 0
;[28:8] str1
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
