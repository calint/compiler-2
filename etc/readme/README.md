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
    * to compile for rv32i and run in QEMU use `--target=rv32i`
* to run the tests `qa/coverage/run-tests-all.sh` and see coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    52           3149            993          11716
C++                              1             46              7            235
-------------------------------------------------------------------------------
SUM:                            53           3195           1000          11951
-------------------------------------------------------------------------------
```

## Sample

```text
# user types are defined using keyword `type`

# built-in types are `i63`, `i32`, `i16`, `i8` and `bool`

# default type is used if ommitted (`i64` on x86_64 and 'i32' on rv32i)

type point {x, y}

type object {pos point, color i32}

type world { locations[8] }

type str {
    len i8,
    data[127] i8
}

# initial data is initialized before variables

dat   hello[] i8 = "hello world from baz\n"
dat prompt1[] i8 = "enter name:\n"
dat prompt2[] i8 = "that is not a name.\n"
dat prompt3[] i8 = "hello "
dat     dot[] i8 = "."
dat      nl[] i8 = "\n"
dat   colon[] i8 = ": "
dat    nums[4] = { 1 } # remaining elements are zeroed
dat    str1 str = { 3 } # remaining fields are zeroed

# default is to inline functions

func assert(x bool) { if not x exit(1) }

func print(str[] i8) {
    write(1, address_of(str), array_size_of(str))
}

# function arguments and return are equivalent to mutable references

func fooz(pt point) {
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

func inv(i i32) res i32 {
    res = ~i
}

func baz(arg) res {
    res = arg * 2
}

# array arguments are declared with type and []

func faz(arg[] i32) {
    arg[1] = 0xfe
}

func str_in(s str) {
    var nbytes = read(0, address_of(s.data), array_size_of(s.data))
    s.len = nbytes - 1
} 

func str_out(s str) {
    write(1, address_of(s.data), s.len)
} 

func point_init() res point {
    res.x = -1
    res.y = -2
}

func object_init() res object {
    res.pos.y = 74
}

const yes = 1
const no = 0
const maybe = -1

# constants can be declared in any scope and shadow outer declarations

# limited support for non-inlined functions

func noinline print_num(num) {
    var buf[20] i8
    var n = num
    var is_negative bool = false
 
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
    var arr[4] i32
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

    var arr1[8] i32
    array_copy(arr, arr1, 4)
    assert(arrays_equal(arr[1], arr1[1], 3))
    # `arrays_equal` is built-in function

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    ix = 3
    arr[ix] = ~inv(arr[ix - 1])
    assert(arr[ix] == 2)

    faz(arr)
    assert(arr[1] == 0xfe)

    var arr3[] = { 3, 5 }
    foo arr3 {
        e = e + i + n
    }
    assert(arr3[0] == 3 + 0 + 2)
    assert(arr3[1] == 5 + 1 + 2)
    # `foo` is a language construct that iterates over an array injecting:
    #   `e`: current element
    #   `i`: index starting at 0
    #   `n`: constant array size

    var p point = {0, 0}
    fooz(p)
    assert(p.x == 2)
    assert(p.y == 0xb)

    var q point = p
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

    var p0 point = {baz(3), 0}
    assert(p0.x == 6)

    var pt point = point_init()
    assert(pt.x == -1)
    assert(pt.y == -2)

    var x = 1
    var y = 2

    var o1 object = {{x * 10, y}, 0xff0000}
    assert(o1.pos.x == 10)
    assert(o1.pos.y == 2)
    assert(o1.color == 0xff0000)

    var p1 point = {-x, -y}
    o1.pos = p1
    assert(o1.pos.x == -1)
    assert(o1.pos.y == -2)

    var o2 object = o1
    assert(o2.pos.x == -1)
    assert(o2.pos.y == -2)
    assert(o2.color == 0xff0000)

    var o3[2] object
    o3.pos.y = 73
    # index 0 in an array can be accessed without array index

    assert(o3[0].pos.y == 73)

    o3[1] = object_init()
    assert(o3[1].pos.y == 74)

    var worlds[8] world
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

    var arr2[] = { -1, 2 }
    assert(array_size_of(arr2) == 2)
    assert(arr2[0] == -1)
    assert(arr2[1] == 2)

    var counter
    var nm str
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
    cmp_137_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_32_26_137_5:
        cmp_32_26_137_5:
        cmp r15b, 0
        jne if_32_23_137_5_end
        if_32_26_137_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_137_5_end:
    assert_137_5_end:
    mov qword [rbp + 240], -1
    cmp_141_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_141_12:
    assert_141_5:
        if_32_26_141_5:
        cmp_32_26_141_5:
        cmp r15b, 0
        jne if_32_23_141_5_end
        if_32_26_141_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_141_5_end:
    assert_141_5_end:
        cmp_145_16:
        bool_end_145_16:
        mov r15b, 1
        assert_145_9:
            if_32_26_145_9:
            cmp_32_26_145_9:
            cmp r15b, 0
            jne if_32_23_145_9_end
            if_32_26_145_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_32_23_145_9_end:
        assert_145_9_end:
    cmp_148_12:
    bool_end_148_12:
    mov r15b, 1
    assert_148_5:
        if_32_26_148_5:
        cmp_32_26_148_5:
        cmp r15b, 0
        jne if_32_23_148_5_end
        if_32_26_148_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_148_5_end:
    assert_148_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 153
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 154
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_155_12:
    mov r14, 1
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_155_12:
    assert_155_5:
        if_32_26_155_5:
        cmp_32_26_155_5:
        cmp r15b, 0
        jne if_32_23_155_5_end
        if_32_26_155_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_155_5_end:
    assert_155_5_end:
    cmp_156_12:
    mov r14, 2
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_32_26_156_5:
        cmp_32_26_156_5:
        cmp r15b, 0
        jne if_32_23_156_5_end
        if_32_26_156_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_156_5_end:
    assert_156_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 158
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 158
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_160_12:
    mov r14, 0
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
        if_32_26_160_5:
        cmp_32_26_160_5:
        cmp r15b, 0
        jne if_32_23_160_5_end
        if_32_26_160_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_160_5_end:
    assert_160_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 163
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 163
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_164_12:
        mov rcx, 3
        mov r14, 1
        mov r13, 164
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        mov r12, rcx
        add r12, r14
        cmp r12, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 * 4 + 224]
        mov r14, 1
        mov r13, 164
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        mov r12, rcx
        add r12, r14
        cmp r12, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + r14 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool_end_164_12:
    assert_164_5:
        if_32_26_164_5:
        cmp_32_26_164_5:
        cmp r15b, 0
        jne if_32_23_164_5_end
        if_32_26_164_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_164_5_end:
    assert_164_5_end:
    mov r15, 2
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_168_12:
        mov rcx, 4
        mov r14, 168
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 168
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool_end_168_12:
    assert_168_5:
        if_32_26_168_5:
        cmp_32_26_168_5:
        cmp r15b, 0
        jne if_32_23_168_5_end
        if_32_26_168_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_168_5_end:
    assert_168_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 171
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_171_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_171_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_172_12:
    mov r14, qword [rbp + 248]
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_32_26_172_5:
        cmp_32_26_172_5:
        cmp r15b, 0
        jne if_32_23_172_5_end
        if_32_26_172_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_172_5_end:
    assert_172_5_end:
    faz_174_5:
        mov r15, 1
        mov r14, 67
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_174_5_end:
    cmp_175_12:
    mov r14, 1
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_175_12:
    assert_175_5:
        if_32_26_175_5:
        cmp_32_26_175_5:
        cmp r15b, 0
        jne if_32_23_175_5_end
        if_32_26_175_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_175_5_end:
    assert_175_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_178_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_178_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_178_5
    foo_178_5_end:
    cmp_181_12:
    mov r14, 0
    mov r13, 181
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
    bool_end_181_12:
    assert_181_5:
        if_32_26_181_5:
        cmp_32_26_181_5:
        cmp r15b, 0
        jne if_32_23_181_5_end
        if_32_26_181_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_181_5_end:
    assert_181_5_end:
    cmp_182_12:
    mov r14, 1
    mov r13, 182
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
    bool_end_182_12:
    assert_182_5:
        if_32_26_182_5:
        cmp_32_26_182_5:
        cmp r15b, 0
        jne if_32_23_182_5_end
        if_32_26_182_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_182_5_end:
    assert_182_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_189_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_189_5_end:
    cmp_190_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_32_26_190_5:
        cmp_32_26_190_5:
        cmp r15b, 0
        jne if_32_23_190_5_end
        if_32_26_190_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_32_26_191_5:
        cmp_32_26_191_5:
        cmp r15b, 0
        jne if_32_23_191_5_end
        if_32_26_191_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_191_5_end:
    assert_191_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_194_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool_end_194_12:
    assert_194_5:
        if_32_26_194_5:
        cmp_32_26_194_5:
        cmp r15b, 0
        jne if_32_23_194_5_end
        if_32_26_194_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_194_5_end:
    assert_194_5_end:
    mov qword [rbp + 320], 3
    cmp_199_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_199_12:
    assert_199_5:
        if_32_26_199_5:
        cmp_32_26_199_5:
        cmp r15b, 0
        jne if_32_23_199_5_end
        if_32_26_199_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_199_5_end:
    assert_199_5_end:
    mov qword [rbp + 336], 0
    bar_202_5:
        if_48_8_202_5:
        cmp_48_8_202_5:
        cmp qword [rbp + 336], 0
        jne if_48_5_202_5_end
        if_48_8_202_5_code:
            jmp bar_202_5_end
        if_48_5_202_5_end:
        mov qword [rbp + 336], 255
    bar_202_5_end:
    cmp_203_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_32_26_203_5:
        cmp_32_26_203_5:
        cmp r15b, 0
        jne if_32_23_203_5_end
        if_32_26_203_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 336], 1
    bar_206_5:
        if_48_8_206_5:
        cmp_48_8_206_5:
        cmp qword [rbp + 336], 0
        jne if_48_5_206_5_end
        if_48_8_206_5_code:
            jmp bar_206_5_end
        if_48_5_206_5_end:
        mov qword [rbp + 336], 255
    bar_206_5_end:
    cmp_207_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_32_26_207_5:
        cmp_32_26_207_5:
        cmp r15b, 0
        jne if_32_23_207_5_end
        if_32_26_207_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_207_5_end:
    assert_207_5_end:
    mov qword [rbp + 344], 1
    baz_210_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_210_13_end:
    cmp_211_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_32_26_211_5:
        cmp_32_26_211_5:
        cmp r15b, 0
        jne if_32_23_211_5_end
        if_32_26_211_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_211_5_end:
    assert_211_5_end:
    baz_213_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_213_9_end:
    cmp_214_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_32_26_214_5:
        cmp_32_26_214_5:
        cmp r15b, 0
        jne if_32_23_214_5_end
        if_32_26_214_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_214_5_end:
    assert_214_5_end:
    baz_216_21:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_216_21_end:
    mov qword [rbp + 368], 0
    cmp_217_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_32_26_217_5:
        cmp_32_26_217_5:
        cmp r15b, 0
        jne if_32_23_217_5_end
        if_32_26_217_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_217_5_end:
    assert_217_5_end:
    point_init_219_20:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_219_20_end:
    cmp_220_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_220_12:
    assert_220_5:
        if_32_26_220_5:
        cmp_32_26_220_5:
        cmp r15b, 0
        jne if_32_23_220_5_end
        if_32_26_220_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_220_5_end:
    assert_220_5_end:
    cmp_221_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_32_26_221_5:
        cmp_32_26_221_5:
        cmp r15b, 0
        jne if_32_23_221_5_end
        if_32_26_221_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_221_5_end:
    assert_221_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_227_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_227_12:
    assert_227_5:
        if_32_26_227_5:
        cmp_32_26_227_5:
        cmp r15b, 0
        jne if_32_23_227_5_end
        if_32_26_227_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_227_5_end:
    assert_227_5_end:
    cmp_228_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_32_26_228_5:
        cmp_32_26_228_5:
        cmp r15b, 0
        jne if_32_23_228_5_end
        if_32_26_228_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_228_5_end:
    assert_228_5_end:
    cmp_229_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_32_26_229_5:
        cmp_32_26_229_5:
        cmp r15b, 0
        jne if_32_23_229_5_end
        if_32_26_229_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_229_5_end:
    assert_229_5_end:
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
    cmp_233_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_32_26_233_5:
        cmp_32_26_233_5:
        cmp r15b, 0
        jne if_32_23_233_5_end
        if_32_26_233_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_233_5_end:
    assert_233_5_end:
    cmp_234_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_32_26_234_5:
        cmp_32_26_234_5:
        cmp r15b, 0
        jne if_32_23_234_5_end
        if_32_26_234_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_234_5_end:
    assert_234_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_237_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_32_26_237_5:
        cmp_32_26_237_5:
        cmp r15b, 0
        jne if_32_23_237_5_end
        if_32_26_237_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_237_5_end:
    assert_237_5_end:
    cmp_238_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_32_26_238_5:
        cmp_32_26_238_5:
        cmp r15b, 0
        jne if_32_23_238_5_end
        if_32_26_238_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_238_5_end:
    assert_238_5_end:
    cmp_239_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_32_26_239_5:
        cmp_32_26_239_5:
        cmp r15b, 0
        jne if_32_23_239_5_end
        if_32_26_239_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_239_5_end:
    assert_239_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_245_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 245
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
    bool_end_245_12:
    assert_245_5:
        if_32_26_245_5:
        cmp_32_26_245_5:
        cmp r15b, 0
        jne if_32_23_245_5_end
        if_32_26_245_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_245_5_end:
    assert_245_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_247_13:
        mov qword [r15 + 8], 74
    object_init_247_13_end:
    cmp_248_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 248
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
    bool_end_248_12:
    assert_248_5:
        if_32_26_248_5:
        cmp_32_26_248_5:
        cmp r15b, 0
        jne if_32_23_248_5_end
        if_32_26_248_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_248_5_end:
    assert_248_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_252_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_252_12:
    assert_252_5:
        if_32_26_252_5:
        cmp_32_26_252_5:
        cmp r15b, 0
        jne if_32_23_252_5_end
        if_32_26_252_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_252_5_end:
    assert_252_5_end:
    mov rcx, 8
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
    mov r14, 255
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 256
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_262_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 262
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 262
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_262_12:
    assert_262_5:
        if_32_26_262_5:
        cmp_32_26_262_5:
        cmp r15b, 0
        jne if_32_23_262_5_end
        if_32_26_262_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_262_5_end:
    assert_262_5_end:
    cmp_263_12:
        mov rcx, 8
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
        mov r13, 264
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [r14]
        lea r14, [rbp + 504]
        mov r13, 1
        mov r12, 265
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 265
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [r14]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool_end_263_12:
    assert_263_5:
        if_32_26_263_5:
        cmp_32_26_263_5:
        cmp r15b, 0
        jne if_32_23_263_5_end
        if_32_26_263_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_263_5_end:
    assert_263_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_270_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_270_12:
    assert_270_5:
        if_32_26_270_5:
        cmp_32_26_270_5:
        cmp r15b, 0
        jne if_32_23_270_5_end
        if_32_26_270_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_270_5_end:
    assert_270_5_end:
    cmp_271_12:
    mov r14, 0
    mov r13, 271
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_271_12:
    assert_271_5:
        if_32_26_271_5:
        cmp_32_26_271_5:
        cmp r15b, 0
        jne if_32_23_271_5_end
        if_32_26_271_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_271_5_end:
    assert_271_5_end:
    cmp_272_12:
    mov r14, 1
    mov r13, 272
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_32_26_272_5:
        cmp_32_26_272_5:
        cmp r15b, 0
        jne if_32_23_272_5_end
        if_32_26_272_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_272_5_end:
    assert_272_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_276_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_276_5_end:
    loop_277_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_280_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_280_9_end:
        print_281_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_281_9_end:
        str_in_282_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_282_9_end:
        if_283_12:
        cmp_283_12:
        cmp byte [rbp + 1040], 0
        jne if_285_19
        if_283_12_code:
            jmp loop_277_5_end
        jmp if_283_9_end
        if_285_19:
        cmp_285_19:
        cmp byte [rbp + 1040], 4
        jg if_else_283_9
        if_285_19_code:
            print_286_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_286_13_end:
            jmp loop_277_5
        jmp if_283_9_end
        if_else_283_9:
            print_289_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_289_13_end:
            str_out_290_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_290_13_end:
            print_291_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_291_13_end:
            print_292_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_292_13_end:
        if_283_9_end:
    jmp loop_277_5
    loop_277_5_end:
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
    if_101_8:
    cmp_101_8:
    cmp qword [rbx + 28], 0
    jge if_101_5_end
    if_101_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_101_5_end:
    mov qword [rbx + 37], 20
    loop_107_5:
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
        mov r14, 111
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
        if_113_12:
        cmp_113_12:
        cmp qword [rbx + 28], 0
        jne if_113_9_end
        if_113_12_code:
            jmp loop_107_5_end
        if_113_9_end:
    jmp loop_107_5
    loop_107_5_end:
    if_116_8:
    cmp_116_8:
    cmp byte [rbx + 36], 0
    je if_116_5_end
    if_116_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 118
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_116_5_end:
    mov qword [rbx + 45], 0
    loop_122_5:
        mov r15, qword [rbx + 45]
        mov r14, 123
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 123
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
        if_126_12:
        cmp_126_12:
        cmp qword [rbx + 37], 20
        jne if_126_9_end
        if_126_12_code:
            jmp loop_122_5_end
        if_126_9_end:
    jmp loop_122_5
    loop_122_5_end:
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
;[20:1] dat hello[] i8 = "hello world from baz\n"
;[20:7] hello: i8[21] (21 B @ [rbp])
;[21:1] dat prompt1[] i8 = "enter name:\n"
;[21:5] prompt1: i8[12] (12 B @ [rbp + 21])
;[22:1] dat prompt2[] i8 = "that is not a name.\n"
;[22:5] prompt2: i8[20] (20 B @ [rbp + 33])
;[23:1] dat prompt3[] i8 = "hello "
;[23:5] prompt3: i8[6] (6 B @ [rbp + 53])
;[24:1] dat dot[] i8 = "."
;[24:9] dot: i8[1] (1 B @ [rbp + 59])
;[25:1] dat nl[] i8 = "\n"
;[25:10] nl: i8[1] (1 B @ [rbp + 60])
;[26:1] dat colon[] i8 = ": "
;[26:7] colon: i8[2] (2 B @ [rbp + 61])
;[27:1] dat nums[4] = { 1 }
;[27:8] nums: i64[4] (32 B @ [rbp + 63])
;[27:24] # remaining elements are zeroed
;[28:1] dat str1 str = { 3 }
;[28:8] str1: str (128 B @ [rbp + 95])
;[28:25] # remaining fields are zeroed
;[30:1] # default is to inline functions
;[38:1] # function arguments and return are equivalent to mutable references
;[45:1] # default argument type is i64 on x86_64 and i32 on rv32i
;[52:1] # return target is specified as a variable, in this case `res`
;[54:1] # return variable is a mutable reference to destination
;[64:1] # array arguments are declared with type and []
;[88:7] const yes = 1
;[89:7] const no = 0
;[90:7] const maybe = -1
;[92:1] # constants can be declared in any scope and shadow outer declarations
;[94:1] # limited support for non-inlined functions
; 
main:
;   [133:5] var arr[4] i32
;   [133:9] arr: i32[4] (16 B @ [rbp + 224])
;   [133:9] zero 4 * 4 B = 16 B
;   [133:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [134:5] # arrays are initialized to 0
;   [136:5] var answer
;   [136:9] answer: i64 (8 B @ [rbp + 240])
;   [136:9] zero 1 * 8 B = 8 B
;   [136:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [137:5] assert(answer == 0)
;   [137:12] allocate scratch register -> r15
;   [137:12] ? answer == 0
;   [137:12] ? answer == 0
    cmp_137_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_137_12:
;   [32:6] assert(x bool)
    assert_137_5:
;       [137:5] alias x -> r15b
        if_32_26_137_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_137_5:
        cmp r15b, 0
        jne if_32_23_137_5_end
        if_32_26_137_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_137_5_end:
;       [137:5] free scratch register r15
    assert_137_5_end:
;   [138:5] # variables without initializer are zeroed
;   [140:5] answer = maybe
;   [140:14] maybe
    mov qword [rbp + 240], -1
;   [141:5] assert(answer == -1)
;   [141:12] allocate scratch register -> r15
;   [141:12] ? answer == -1
;   [141:12] ? answer == -1
    cmp_141_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_141_12:
;   [32:6] assert(x bool)
    assert_141_5:
;       [141:5] alias x -> r15b
        if_32_26_141_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_141_5:
        cmp r15b, 0
        jne if_32_23_141_5_end
        if_32_26_141_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_141_5_end:
;       [141:5] free scratch register r15
    assert_141_5_end:
;       [144:15] const maybe = 33
;       [145:9] assert(maybe == 33)
;       [145:16] allocate scratch register -> r15
;       [145:16] ? maybe == 33
;       [145:16] ? maybe == 33
        cmp_145_16:
;       [145:16] const eval to true
        bool_end_145_16:
        mov r15b, 1
;       [32:6] assert(x bool)
        assert_145_9:
;           [145:9] alias x -> r15b
            if_32_26_145_9:
;           [32:26] ? not x
;           [32:26] ? not x
            cmp_32_26_145_9:
            cmp r15b, 0
            jne if_32_23_145_9_end
            if_32_26_145_9_code:
;               [32:32] exit(1)
;               [32:32] allocate named register rdi
;               [32:37] 1
                mov rdi, 1
                mov rax, 60
                syscall
;               [32:32] free named register rdi
            if_32_23_145_9_end:
;           [145:9] free scratch register r15
        assert_145_9_end:
;   [148:5] assert(maybe == -1)
;   [148:12] allocate scratch register -> r15
;   [148:12] ? maybe == -1
;   [148:12] ? maybe == -1
    cmp_148_12:
;   [148:12] const eval to true
    bool_end_148_12:
    mov r15b, 1
;   [32:6] assert(x bool)
    assert_148_5:
;       [148:5] alias x -> r15b
        if_32_26_148_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_148_5:
        cmp r15b, 0
        jne if_32_23_148_5_end
        if_32_26_148_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_148_5_end:
;       [148:5] free scratch register r15
    assert_148_5_end:
;   [150:5] var ix = 1
;   [150:9] ix: i64 (8 B @ [rbp + 248])
;   [150:9] ix = 1
;   [150:14] 1
    mov qword [rbp + 248], 1
;   [151:5] # variables can have an initial value that can be an expression
;   [153:5] arr[ix] = 2
;   [153:5] allocate scratch register -> r15
;   [153:9] set array index
;   [153:9] ix
    mov r15, qword [rbp + 248]
;   [153:9] bounds check
;   [153:9] allocate scratch register -> r14
;   [153:9] line number
    mov r14, 153
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [153:9] free scratch register r14
;   [153:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [153:5] free scratch register r15
;   [154:5] arr[ix + 1] = arr[ix]
;   [154:5] allocate scratch register -> r15
;   [154:9] set array index
;   [154:9] ix
    mov r15, qword [rbp + 248]
;   [154:14] r15 + 1
    add r15, 1
;   [154:9] bounds check
;   [154:9] allocate scratch register -> r14
;   [154:9] line number
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [154:9] free scratch register r14
;   [154:19] arr[ix]
;   [154:19] allocate scratch register -> r14
;   [154:23] set array index
;   [154:23] ix
    mov r14, qword [rbp + 248]
;   [154:23] bounds check
;   [154:23] allocate scratch register -> r13
;   [154:23] line number
    mov r13, 154
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [154:23] free scratch register r13
;   [154:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [154:19] free scratch register r13
;   [154:19] free scratch register r14
;   [154:5] free scratch register r15
;   [155:5] assert(arr[1] == 2)
;   [155:12] allocate scratch register -> r15
;   [155:12] ? arr[1] == 2
;   [155:12] ? arr[1] == 2
    cmp_155_12:
;   [155:12] allocate scratch register -> r14
;   [155:16] set array index
;   [155:16] 1
    mov r14, 1
;   [155:16] bounds check
;   [155:16] allocate scratch register -> r13
;   [155:16] line number
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [155:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [155:12] free scratch register r14
    sete r15b
    bool_end_155_12:
;   [32:6] assert(x bool)
    assert_155_5:
;       [155:5] alias x -> r15b
        if_32_26_155_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_155_5:
        cmp r15b, 0
        jne if_32_23_155_5_end
        if_32_26_155_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_155_5_end:
;       [155:5] free scratch register r15
    assert_155_5_end:
;   [156:5] assert(arr[2] == 2)
;   [156:12] allocate scratch register -> r15
;   [156:12] ? arr[2] == 2
;   [156:12] ? arr[2] == 2
    cmp_156_12:
;   [156:12] allocate scratch register -> r14
;   [156:16] set array index
;   [156:16] 2
    mov r14, 2
;   [156:16] bounds check
;   [156:16] allocate scratch register -> r13
;   [156:16] line number
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [156:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [156:12] free scratch register r14
    sete r15b
    bool_end_156_12:
;   [32:6] assert(x bool)
    assert_156_5:
;       [156:5] alias x -> r15b
        if_32_26_156_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_156_5:
        cmp r15b, 0
        jne if_32_23_156_5_end
        if_32_26_156_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_156_5_end:
;       [156:5] free scratch register r15
    assert_156_5_end:
;   [158:5] array_copy(arr[2], arr, 2)
;   [158:5] allocate named register rsi
;   [158:5] allocate named register rdi
;   [158:5] allocate named register rcx
;   [158:29] 2
;   [158:29] 2
    mov rcx, 2
;   [158:16] arr[2]
;   [158:16] allocate scratch register -> r15
;   [158:20] set array index
;   [158:20] 2
    mov r15, 2
;   [158:20] bounds check
;   [158:20] allocate scratch register -> r14
;   [158:20] line number
    mov r14, 158
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [158:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [158:20] free scratch register r13
    cmovg rbp, r14
    jg baz_bounds_panic
;   [158:20] free scratch register r14
    lea rsi, [rbp + r15 * 4 + 224]
;   [158:5] free scratch register r15
;   [158:24] arr
;   [158:24] bounds check
;   [158:24] allocate scratch register -> r15
;   [158:24] line number
    mov r15, 158
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [158:24] free scratch register r15
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
;   [158:5] free named register rcx
;   [158:5] free named register rdi
;   [158:5] free named register rsi
;   [159:5] # copy from, to, number of elements
;   [160:5] assert(arr[0] == 2)
;   [160:12] allocate scratch register -> r15
;   [160:12] ? arr[0] == 2
;   [160:12] ? arr[0] == 2
    cmp_160_12:
;   [160:12] allocate scratch register -> r14
;   [160:16] set array index
;   [160:16] 0
    mov r14, 0
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
;   [32:6] assert(x bool)
    assert_160_5:
;       [160:5] alias x -> r15b
        if_32_26_160_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_160_5:
        cmp r15b, 0
        jne if_32_23_160_5_end
        if_32_26_160_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_160_5_end:
;       [160:5] free scratch register r15
    assert_160_5_end:
;   [162:5] var arr1[8] i32
;   [162:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [162:9] zero 8 * 4 B = 32 B
;   [162:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [163:5] array_copy(arr, arr1, 4)
;   [163:5] allocate named register rsi
;   [163:5] allocate named register rdi
;   [163:5] allocate named register rcx
;   [163:27] 4
;   [163:27] 4
    mov rcx, 4
;   [163:16] arr
;   [163:16] bounds check
;   [163:16] allocate scratch register -> r15
;   [163:16] line number
    mov r15, 163
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [163:16] free scratch register r15
    lea rsi, [rbp + 224]
;   [163:21] arr1
;   [163:21] bounds check
;   [163:21] allocate scratch register -> r15
;   [163:21] line number
    mov r15, 163
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [163:21] free scratch register r15
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
;   [163:5] free named register rcx
;   [163:5] free named register rdi
;   [163:5] free named register rsi
;   [164:5] assert(arrays_equal(arr[1], arr1[1], 3))
;   [164:12] allocate scratch register -> r15
;   [164:12] ? arrays_equal(arr[1], arr1[1], 3)
;   [164:12] ? arrays_equal(arr[1], arr1[1], 3)
    cmp_164_12:
;       [164:12] arrays_equal(arr[1], arr1[1], 3)
;       [164:12] allocate named register rsi
;       [164:12] allocate named register rdi
;       [164:12] allocate named register rcx
;       [164:42] 3
;       [164:42] 3
        mov rcx, 3
;       [164:25] arr[1]
;       [164:25] allocate scratch register -> r14
;       [164:29] set array index
;       [164:29] 1
        mov r14, 1
;       [164:29] bounds check
;       [164:29] allocate scratch register -> r13
;       [164:29] line number
        mov r13, 164
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
;       [164:29] allocate scratch register -> r12
        mov r12, rcx
        add r12, r14
        cmp r12, 4
;       [164:29] free scratch register r12
        cmovg rbp, r13
        jg baz_bounds_panic
;       [164:29] free scratch register r13
        lea rsi, [rbp + r14 * 4 + 224]
;       [164:12] free scratch register r14
;       [164:33] arr1[1]
;       [164:33] allocate scratch register -> r14
;       [164:38] set array index
;       [164:38] 1
        mov r14, 1
;       [164:38] bounds check
;       [164:38] allocate scratch register -> r13
;       [164:38] line number
        mov r13, 164
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
;       [164:38] allocate scratch register -> r12
        mov r12, rcx
        add r12, r14
        cmp r12, 8
;       [164:38] free scratch register r12
        cmovg rbp, r13
        jg baz_bounds_panic
;       [164:38] free scratch register r13
        lea rdi, [rbp + r14 * 4 + 256]
;       [164:12] free scratch register r14
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [164:12] free named register rcx
;       [164:12] free named register rdi
;       [164:12] free named register rsi
        sete r15b
    bool_end_164_12:
;   [32:6] assert(x bool)
    assert_164_5:
;       [164:5] alias x -> r15b
        if_32_26_164_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_164_5:
        cmp r15b, 0
        jne if_32_23_164_5_end
        if_32_26_164_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_164_5_end:
;       [164:5] free scratch register r15
    assert_164_5_end:
;   [165:5] # `arrays_equal` is built-in function
;   [167:5] arr1[2] = -1
;   [167:5] allocate scratch register -> r15
;   [167:10] set array index
;   [167:10] 2
    mov r15, 2
;   [167:10] bounds check
;   [167:10] allocate scratch register -> r14
;   [167:10] line number
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [167:10] free scratch register r14
;   [167:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [167:5] free scratch register r15
;   [168:5] assert(not arrays_equal(arr, arr1, 4))
;   [168:12] allocate scratch register -> r15
;   [168:12] ? not arrays_equal(arr, arr1, 4)
;   [168:12] ? not arrays_equal(arr, arr1, 4)
    cmp_168_12:
;       [168:16] arrays_equal(arr, arr1, 4)
;       [168:16] allocate named register rsi
;       [168:16] allocate named register rdi
;       [168:16] allocate named register rcx
;       [168:40] 4
;       [168:40] 4
        mov rcx, 4
;       [168:29] arr
;       [168:29] bounds check
;       [168:29] allocate scratch register -> r14
;       [168:29] line number
        mov r14, 168
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
;       [168:29] free scratch register r14
        lea rsi, [rbp + 224]
;       [168:34] arr1
;       [168:34] bounds check
;       [168:34] allocate scratch register -> r14
;       [168:34] line number
        mov r14, 168
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [168:34] free scratch register r14
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [168:16] free named register rcx
;       [168:16] free named register rdi
;       [168:16] free named register rsi
        setne r15b
    bool_end_168_12:
;   [32:6] assert(x bool)
    assert_168_5:
;       [168:5] alias x -> r15b
        if_32_26_168_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_168_5:
        cmp r15b, 0
        jne if_32_23_168_5_end
        if_32_26_168_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_168_5_end:
;       [168:5] free scratch register r15
    assert_168_5_end:
;   [170:5] ix = 3
;   [170:10] 3
    mov qword [rbp + 248], 3
;   [171:5] arr[ix] = ~inv(arr[ix - 1])
;   [171:5] allocate scratch register -> r15
;   [171:9] set array index
;   [171:9] ix
    mov r15, qword [rbp + 248]
;   [171:9] bounds check
;   [171:9] allocate scratch register -> r14
;   [171:9] line number
    mov r14, 171
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [171:9] free scratch register r14
;   [171:16] arr = ~inv(arr[ix - 1])
;   [171:16] = expression
;   [171:16] ~inv(arr[ix - 1])
;   [171:20] allocate scratch register -> r14
;   [171:24] set array index
;   [171:24] ix
    mov r14, qword [rbp + 248]
;   [171:29] r14 - 1
    sub r14, 1
;   [171:24] bounds check
;   [171:24] allocate scratch register -> r13
;   [171:24] line number
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [171:24] free scratch register r13
;   [56:6] inv(i i32) res i32
    inv_171_16:
;       [171:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [171:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [57:5] res = ~i
;       [57:12] ~i
;       [57:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [57:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [171:16] free scratch register r14
    inv_171_16_end:
    not dword [rbp + r15 * 4 + 224]
;   [171:5] free scratch register r15
;   [172:5] assert(arr[ix] == 2)
;   [172:12] allocate scratch register -> r15
;   [172:12] ? arr[ix] == 2
;   [172:12] ? arr[ix] == 2
    cmp_172_12:
;   [172:12] allocate scratch register -> r14
;   [172:16] set array index
;   [172:16] ix
    mov r14, qword [rbp + 248]
;   [172:16] bounds check
;   [172:16] allocate scratch register -> r13
;   [172:16] line number
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [172:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [172:12] free scratch register r14
    sete r15b
    bool_end_172_12:
;   [32:6] assert(x bool)
    assert_172_5:
;       [172:5] alias x -> r15b
        if_32_26_172_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_172_5:
        cmp r15b, 0
        jne if_32_23_172_5_end
        if_32_26_172_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_172_5_end:
;       [172:5] free scratch register r15
    assert_172_5_end:
;   [174:5] faz(arr)
;   [66:6] faz(arg[] i32)
    faz_174_5:
;       [174:5] alias arg -> arr
;       [67:5] arg[1] = 0xfe
;       [67:5] allocate scratch register -> r15
;       [67:9] set array index
;       [67:9] 1
        mov r15, 1
;       [67:9] bounds check
;       [67:9] allocate scratch register -> r14
;       [67:9] line number
        mov r14, 67
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
;       [67:9] free scratch register r14
;       [67:14] 0xfe
        mov dword [rbp + r15 * 4 + 224], 254
;       [67:5] free scratch register r15
    faz_174_5_end:
;   [175:5] assert(arr[1] == 0xfe)
;   [175:12] allocate scratch register -> r15
;   [175:12] ? arr[1] == 0xfe
;   [175:12] ? arr[1] == 0xfe
    cmp_175_12:
;   [175:12] allocate scratch register -> r14
;   [175:16] set array index
;   [175:16] 1
    mov r14, 1
;   [175:16] bounds check
;   [175:16] allocate scratch register -> r13
;   [175:16] line number
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [175:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 254
;   [175:12] free scratch register r14
    sete r15b
    bool_end_175_12:
;   [32:6] assert(x bool)
    assert_175_5:
;       [175:5] alias x -> r15b
        if_32_26_175_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_175_5:
        cmp r15b, 0
        jne if_32_23_175_5_end
        if_32_26_175_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_175_5_end:
;       [175:5] free scratch register r15
    assert_175_5_end:
;   [177:5] var arr3[] = { 3, 5 }
;   [177:9] arr3: i64[2] (16 B @ [rbp + 288])
;   [177:9] arr3= { 3, 5 }
;   [177:20] [0]
;   [177:20] 3
    mov qword [rbp + 288], 3
;   [177:20] [1]
;   [177:23] 5
    mov qword [rbp + 296], 5
;   [178:5] foo arr3
;   [178:9] allocate scratch register -> r15
;   [178:9] e: i64 (r15)
;   [178:9] i: i64 (8 B @ [rbp + 312])
;   [178:9] const n = 2
;   [178:9] initiate iterator e
    lea r15, [rbp + 288]
;   [178:9] initiate counter i
    mov qword [rbp + 312], 0
    foo_178_5:
;       [179:9] e = e + i + n
;       [179:13] instructions without scratch register 3, with 4
;       [179:13] e
;       [179:17] e + i
;       [179:17] allocate scratch register -> r14
        mov r14, qword [rbp + 312]
        add qword [r15], r14
;       [179:17] free scratch register r14
;       [179:21] e + n
        add qword [r15], 2
        foo_178_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_178_5
    foo_178_5_end:
;   [178:5] free scratch register r15
;   [181:5] assert(arr3[0] == 3 + 0 + 2)
;   [181:12] allocate scratch register -> r15
;   [181:12] ? arr3[0] == 3 + 0 + 2
;   [181:12] ? arr3[0] == 3 + 0 + 2
    cmp_181_12:
;   [181:12] allocate scratch register -> r14
;   [181:17] set array index
;   [181:17] 0
    mov r14, 0
;   [181:17] bounds check
;   [181:17] allocate scratch register -> r13
;   [181:17] line number
    mov r13, 181
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [181:17] free scratch register r13
;   [181:23] allocate scratch register -> r13
;       [181:23] 3
        mov r13, 3
;       [181:27] r13 + 0
        add r13, 0
;       [181:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [181:12] free scratch register r13
;   [181:12] free scratch register r14
    sete r15b
    bool_end_181_12:
;   [32:6] assert(x bool)
    assert_181_5:
;       [181:5] alias x -> r15b
        if_32_26_181_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_181_5:
        cmp r15b, 0
        jne if_32_23_181_5_end
        if_32_26_181_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_181_5_end:
;       [181:5] free scratch register r15
    assert_181_5_end:
;   [182:5] assert(arr3[1] == 5 + 1 + 2)
;   [182:12] allocate scratch register -> r15
;   [182:12] ? arr3[1] == 5 + 1 + 2
;   [182:12] ? arr3[1] == 5 + 1 + 2
    cmp_182_12:
;   [182:12] allocate scratch register -> r14
;   [182:17] set array index
;   [182:17] 1
    mov r14, 1
;   [182:17] bounds check
;   [182:17] allocate scratch register -> r13
;   [182:17] line number
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [182:17] free scratch register r13
;   [182:23] allocate scratch register -> r13
;       [182:23] 5
        mov r13, 5
;       [182:27] r13 + 1
        add r13, 1
;       [182:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [182:12] free scratch register r13
;   [182:12] free scratch register r14
    sete r15b
    bool_end_182_12:
;   [32:6] assert(x bool)
    assert_182_5:
;       [182:5] alias x -> r15b
        if_32_26_182_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_182_5:
        cmp r15b, 0
        jne if_32_23_182_5_end
        if_32_26_182_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_182_5_end:
;       [182:5] free scratch register r15
    assert_182_5_end:
;   [183:5] # `foo` is a language construct that iterates over an array injecting:
;   [184:5] # `e`: current element
;   [185:5] # `i`: index starting at 0
;   [186:5] # `n`: constant array size
;   [188:5] var p point = {0, 0}
;   [188:9] p: point (16 B @ [rbp + 304])
;   [188:9] p = {0, 0}
;   [188:20] copy field 'x'
    mov qword [rbp + 304], 0
;   [188:23] copy field 'y'
    mov qword [rbp + 312], 0
;   [189:5] fooz(p)
;   [40:6] fooz(pt point)
    fooz_189_5:
;       [189:5] alias pt -> p
;       [41:5] pt.x = 0b10
;       [41:12] 0b10
        mov qword [rbp + 304], 2
;       [41:20] # binary value 2
;       [42:5] pt.y = 0xb
;       [42:12] 0xb
        mov qword [rbp + 312], 11
;       [42:20] # hex value 11
    fooz_189_5_end:
;   [190:5] assert(p.x == 2)
;   [190:12] allocate scratch register -> r15
;   [190:12] ? p.x == 2
;   [190:12] ? p.x == 2
    cmp_190_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_190_12:
;   [32:6] assert(x bool)
    assert_190_5:
;       [190:5] alias x -> r15b
        if_32_26_190_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_190_5:
        cmp r15b, 0
        jne if_32_23_190_5_end
        if_32_26_190_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_190_5_end:
;       [190:5] free scratch register r15
    assert_190_5_end:
;   [191:5] assert(p.y == 0xb)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? p.y == 0xb
;   [191:12] ? p.y == 0xb
    cmp_191_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_191_12:
;   [32:6] assert(x bool)
    assert_191_5:
;       [191:5] alias x -> r15b
        if_32_26_191_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_191_5:
        cmp r15b, 0
        jne if_32_23_191_5_end
        if_32_26_191_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_191_5_end:
;       [191:5] free scratch register r15
    assert_191_5_end:
;   [193:5] var q point = p
;   [193:9] q: point (16 B @ [rbp + 320])
;   [193:9] q = p
;   [193:19] size <= 16 B, use mov
;   [193:19] allocate named register rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
;   [193:19] free named register rax
;   [194:5] assert(equal(p, q))
;   [194:12] allocate scratch register -> r15
;   [194:12] ? equal(p, q)
;   [194:12] ? equal(p, q)
    cmp_194_12:
;       [194:12] equal(p, q)
;       [194:12] allocate named register rsi
;       [194:12] allocate named register rdi
;       [194:12] allocate named register rcx
;       [194:18] p
        lea rsi, [rbp + 304]
;       [194:21] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [194:12] free named register rcx
;       [194:12] free named register rdi
;       [194:12] free named register rsi
        sete r15b
    bool_end_194_12:
;   [32:6] assert(x bool)
    assert_194_5:
;       [194:5] alias x -> r15b
        if_32_26_194_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_194_5:
        cmp r15b, 0
        jne if_32_23_194_5_end
        if_32_26_194_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_194_5_end:
;       [194:5] free scratch register r15
    assert_194_5_end:
;   [195:5] # `equal` is built-in function to compare user types for equality or same
;   [196:5] # size arrays
;   [198:5] q.x = 3
;   [198:11] 3
    mov qword [rbp + 320], 3
;   [199:5] assert(not equal(p, q))
;   [199:12] allocate scratch register -> r15
;   [199:12] ? not equal(p, q)
;   [199:12] ? not equal(p, q)
    cmp_199_12:
;       [199:16] equal(p, q)
;       [199:16] allocate named register rsi
;       [199:16] allocate named register rdi
;       [199:16] allocate named register rcx
;       [199:22] p
        lea rsi, [rbp + 304]
;       [199:25] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [199:16] free named register rcx
;       [199:16] free named register rdi
;       [199:16] free named register rsi
        setne r15b
    bool_end_199_12:
;   [32:6] assert(x bool)
    assert_199_5:
;       [199:5] alias x -> r15b
        if_32_26_199_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_199_5:
        cmp r15b, 0
        jne if_32_23_199_5_end
        if_32_26_199_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_199_5_end:
;       [199:5] free scratch register r15
    assert_199_5_end:
;   [201:5] var i = 0
;   [201:9] i: i64 (8 B @ [rbp + 336])
;   [201:9] i = 0
;   [201:13] 0
    mov qword [rbp + 336], 0
;   [202:5] bar(i)
;   [47:6] bar(arg)
    bar_202_5:
;       [202:5] alias arg -> i
        if_48_8_202_5:
;       [48:8] ? arg == 0
;       [48:8] ? arg == 0
        cmp_48_8_202_5:
        cmp qword [rbp + 336], 0
        jne if_48_5_202_5_end
        if_48_8_202_5_code:
;           [48:17] return
            jmp bar_202_5_end
        if_48_5_202_5_end:
;       [49:5] arg = 0xff
;       [49:11] 0xff
        mov qword [rbp + 336], 255
    bar_202_5_end:
;   [203:5] assert(i == 0)
;   [203:12] allocate scratch register -> r15
;   [203:12] ? i == 0
;   [203:12] ? i == 0
    cmp_203_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_203_12:
;   [32:6] assert(x bool)
    assert_203_5:
;       [203:5] alias x -> r15b
        if_32_26_203_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_203_5:
        cmp r15b, 0
        jne if_32_23_203_5_end
        if_32_26_203_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_203_5_end:
;       [203:5] free scratch register r15
    assert_203_5_end:
;   [205:5] i = 1
;   [205:9] 1
    mov qword [rbp + 336], 1
;   [206:5] bar(i)
;   [47:6] bar(arg)
    bar_206_5:
;       [206:5] alias arg -> i
        if_48_8_206_5:
;       [48:8] ? arg == 0
;       [48:8] ? arg == 0
        cmp_48_8_206_5:
        cmp qword [rbp + 336], 0
        jne if_48_5_206_5_end
        if_48_8_206_5_code:
;           [48:17] return
            jmp bar_206_5_end
        if_48_5_206_5_end:
;       [49:5] arg = 0xff
;       [49:11] 0xff
        mov qword [rbp + 336], 255
    bar_206_5_end:
;   [207:5] assert(i == 0xff)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? i == 0xff
;   [207:12] ? i == 0xff
    cmp_207_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_207_12:
;   [32:6] assert(x bool)
    assert_207_5:
;       [207:5] alias x -> r15b
        if_32_26_207_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_207_5:
        cmp r15b, 0
        jne if_32_23_207_5_end
        if_32_26_207_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_207_5_end:
;       [207:5] free scratch register r15
    assert_207_5_end:
;   [209:5] var j = 1
;   [209:9] j: i64 (8 B @ [rbp + 344])
;   [209:9] j = 1
;   [209:13] 1
    mov qword [rbp + 344], 1
;   [210:5] var k = baz(j)
;   [210:9] k: i64 (8 B @ [rbp + 352])
;   [210:9] k = baz(j)
;   [210:13] k = baz(j)
;   [210:13] = expression
;   [210:13] baz(j)
;   [60:6] baz(arg) res
    baz_210_13:
;       [210:13] alias res -> k
;       [210:13] alias arg -> j
;       [61:5] res = arg * 2
;       [61:11] instructions without scratch register 5, with 3
;       [61:11] allocate scratch register -> r15
;       [61:11] arg
        mov r15, qword [rbp + 344]
;       [61:17] r15 * 2
;       [61:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [61:11] free scratch register r15
    baz_210_13_end:
;   [211:5] assert(k == 2)
;   [211:12] allocate scratch register -> r15
;   [211:12] ? k == 2
;   [211:12] ? k == 2
    cmp_211_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_211_12:
;   [32:6] assert(x bool)
    assert_211_5:
;       [211:5] alias x -> r15b
        if_32_26_211_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_211_5:
        cmp r15b, 0
        jne if_32_23_211_5_end
        if_32_26_211_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_211_5_end:
;       [211:5] free scratch register r15
    assert_211_5_end:
;   [213:5] k = baz(1)
;   [213:9] k = baz(1)
;   [213:9] = expression
;   [213:9] baz(1)
;   [60:6] baz(arg) res
    baz_213_9:
;       [213:9] alias res -> k
;       [213:9] alias arg -> 1
;       [61:5] res = arg * 2
;       [61:11] instructions without scratch register 4, with 3
;       [61:11] allocate scratch register -> r15
;       [61:11] arg
        mov r15, 1
;       [61:17] r15 * 2
;       [61:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [61:11] free scratch register r15
    baz_213_9_end:
;   [214:5] assert(k == 2)
;   [214:12] allocate scratch register -> r15
;   [214:12] ? k == 2
;   [214:12] ? k == 2
    cmp_214_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_214_12:
;   [32:6] assert(x bool)
    assert_214_5:
;       [214:5] alias x -> r15b
        if_32_26_214_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_214_5:
        cmp r15b, 0
        jne if_32_23_214_5_end
        if_32_26_214_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_214_5_end:
;       [214:5] free scratch register r15
    assert_214_5_end:
;   [216:5] var p0 point = {baz(3), 0}
;   [216:9] p0: point (16 B @ [rbp + 360])
;   [216:9] p0 = {baz(3), 0}
;   [216:21] copy field 'x'
;   [216:21] p0.x = baz(3)
;   [216:21] = expression
;   [216:21] baz(3)
;   [60:6] baz(arg) res
    baz_216_21:
;       [216:21] alias res -> p0.x (lea: rbp + 360)
;       [216:21] alias arg -> 3
;       [61:5] res = arg * 2
;       [61:11] instructions without scratch register 4, with 3
;       [61:11] allocate scratch register -> r15
;       [61:11] arg
        mov r15, 3
;       [61:17] r15 * 2
;       [61:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 360], r15
;       [61:11] free scratch register r15
    baz_216_21_end:
;   [216:29] copy field 'y'
    mov qword [rbp + 368], 0
;   [217:5] assert(p0.x == 6)
;   [217:12] allocate scratch register -> r15
;   [217:12] ? p0.x == 6
;   [217:12] ? p0.x == 6
    cmp_217_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_217_12:
;   [32:6] assert(x bool)
    assert_217_5:
;       [217:5] alias x -> r15b
        if_32_26_217_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_217_5:
        cmp r15b, 0
        jne if_32_23_217_5_end
        if_32_26_217_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_217_5_end:
;       [217:5] free scratch register r15
    assert_217_5_end:
;   [219:5] var pt point = point_init()
;   [219:9] pt: point (16 B @ [rbp + 376])
;   [219:9] pt = point_init()
;   [219:20] point_init()
;   [79:6] point_init() res point
    point_init_219_20:
;       [219:20] alias res -> pt
;       [80:5] res.x = -1
;       [80:14] -1
        mov qword [rbp + 376], -1
;       [81:5] res.y = -2
;       [81:14] -2
        mov qword [rbp + 384], -2
    point_init_219_20_end:
;   [220:5] assert(pt.x == -1)
;   [220:12] allocate scratch register -> r15
;   [220:12] ? pt.x == -1
;   [220:12] ? pt.x == -1
    cmp_220_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_220_12:
;   [32:6] assert(x bool)
    assert_220_5:
;       [220:5] alias x -> r15b
        if_32_26_220_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_220_5:
        cmp r15b, 0
        jne if_32_23_220_5_end
        if_32_26_220_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_220_5_end:
;       [220:5] free scratch register r15
    assert_220_5_end:
;   [221:5] assert(pt.y == -2)
;   [221:12] allocate scratch register -> r15
;   [221:12] ? pt.y == -2
;   [221:12] ? pt.y == -2
    cmp_221_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_221_12:
;   [32:6] assert(x bool)
    assert_221_5:
;       [221:5] alias x -> r15b
        if_32_26_221_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_221_5:
        cmp r15b, 0
        jne if_32_23_221_5_end
        if_32_26_221_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_221_5_end:
;       [221:5] free scratch register r15
    assert_221_5_end:
;   [223:5] var x = 1
;   [223:9] x: i64 (8 B @ [rbp + 392])
;   [223:9] x = 1
;   [223:13] 1
    mov qword [rbp + 392], 1
;   [224:5] var y = 2
;   [224:9] y: i64 (8 B @ [rbp + 400])
;   [224:9] y = 2
;   [224:13] 2
    mov qword [rbp + 400], 2
;   [226:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [226:9] o1: object (20 B @ [rbp + 408])
;   [226:9] o1 = {{x * 10, y}, 0xff0000}
;   [226:22] copy field 'pos'
;   [226:23] copy field 'x'
;   [226:23] instructions without scratch register 5, with 3
;   [226:23] allocate scratch register -> r15
;   [226:23] x
    mov r15, qword [rbp + 392]
;   [226:27] r15 * 10
;   [226:27] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 408], r15
;   [226:23] free scratch register r15
;   [226:31] copy field 'y'
;   [226:31] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
;   [226:31] free scratch register r15
;   [226:35] copy field 'color'
    mov dword [rbp + 424], 16711680
;   [227:5] assert(o1.pos.x == 10)
;   [227:12] allocate scratch register -> r15
;   [227:12] ? o1.pos.x == 10
;   [227:12] ? o1.pos.x == 10
    cmp_227_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_227_12:
;   [32:6] assert(x bool)
    assert_227_5:
;       [227:5] alias x -> r15b
        if_32_26_227_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_227_5:
        cmp r15b, 0
        jne if_32_23_227_5_end
        if_32_26_227_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_227_5_end:
;       [227:5] free scratch register r15
    assert_227_5_end:
;   [228:5] assert(o1.pos.y == 2)
;   [228:12] allocate scratch register -> r15
;   [228:12] ? o1.pos.y == 2
;   [228:12] ? o1.pos.y == 2
    cmp_228_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_228_12:
;   [32:6] assert(x bool)
    assert_228_5:
;       [228:5] alias x -> r15b
        if_32_26_228_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_228_5:
        cmp r15b, 0
        jne if_32_23_228_5_end
        if_32_26_228_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_228_5_end:
;       [228:5] free scratch register r15
    assert_228_5_end:
;   [229:5] assert(o1.color == 0xff0000)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? o1.color == 0xff0000
;   [229:12] ? o1.color == 0xff0000
    cmp_229_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_229_12:
;   [32:6] assert(x bool)
    assert_229_5:
;       [229:5] alias x -> r15b
        if_32_26_229_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_229_5:
        cmp r15b, 0
        jne if_32_23_229_5_end
        if_32_26_229_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_229_5_end:
;       [229:5] free scratch register r15
    assert_229_5_end:
;   [231:5] var p1 point = {-x, -y}
;   [231:9] p1: point (16 B @ [rbp + 428])
;   [231:9] p1 = {-x, -y}
;   [231:21] copy field 'x'
;   [231:21] allocate scratch register -> r15
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
;   [231:21] free scratch register r15
    neg qword [rbp + 428]
;   [231:25] copy field 'y'
;   [231:25] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
;   [231:25] free scratch register r15
    neg qword [rbp + 436]
;   [232:5] o1.pos = p1
;   [232:14] size <= 16 B, use mov
;   [232:14] allocate named register rax
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
;   [232:14] free named register rax
;   [233:5] assert(o1.pos.x == -1)
;   [233:12] allocate scratch register -> r15
;   [233:12] ? o1.pos.x == -1
;   [233:12] ? o1.pos.x == -1
    cmp_233_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_233_12:
;   [32:6] assert(x bool)
    assert_233_5:
;       [233:5] alias x -> r15b
        if_32_26_233_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_233_5:
        cmp r15b, 0
        jne if_32_23_233_5_end
        if_32_26_233_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_233_5_end:
;       [233:5] free scratch register r15
    assert_233_5_end:
;   [234:5] assert(o1.pos.y == -2)
;   [234:12] allocate scratch register -> r15
;   [234:12] ? o1.pos.y == -2
;   [234:12] ? o1.pos.y == -2
    cmp_234_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_234_12:
;   [32:6] assert(x bool)
    assert_234_5:
;       [234:5] alias x -> r15b
        if_32_26_234_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_234_5:
        cmp r15b, 0
        jne if_32_23_234_5_end
        if_32_26_234_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_234_5_end:
;       [234:5] free scratch register r15
    assert_234_5_end:
;   [236:5] var o2 object = o1
;   [236:9] o2: object (20 B @ [rbp + 444])
;   [236:9] o2 = o1
;   [236:21] allocate named register rsi
;   [236:21] allocate named register rdi
;   [236:21] allocate named register rcx
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
;   [236:21] free named register rcx
;   [236:21] free named register rdi
;   [236:21] free named register rsi
;   [237:5] assert(o2.pos.x == -1)
;   [237:12] allocate scratch register -> r15
;   [237:12] ? o2.pos.x == -1
;   [237:12] ? o2.pos.x == -1
    cmp_237_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_237_12:
;   [32:6] assert(x bool)
    assert_237_5:
;       [237:5] alias x -> r15b
        if_32_26_237_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_237_5:
        cmp r15b, 0
        jne if_32_23_237_5_end
        if_32_26_237_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_237_5_end:
;       [237:5] free scratch register r15
    assert_237_5_end:
;   [238:5] assert(o2.pos.y == -2)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? o2.pos.y == -2
;   [238:12] ? o2.pos.y == -2
    cmp_238_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_238_12:
;   [32:6] assert(x bool)
    assert_238_5:
;       [238:5] alias x -> r15b
        if_32_26_238_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_238_5:
        cmp r15b, 0
        jne if_32_23_238_5_end
        if_32_26_238_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_238_5_end:
;       [238:5] free scratch register r15
    assert_238_5_end:
;   [239:5] assert(o2.color == 0xff0000)
;   [239:12] allocate scratch register -> r15
;   [239:12] ? o2.color == 0xff0000
;   [239:12] ? o2.color == 0xff0000
    cmp_239_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_239_12:
;   [32:6] assert(x bool)
    assert_239_5:
;       [239:5] alias x -> r15b
        if_32_26_239_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_239_5:
        cmp r15b, 0
        jne if_32_23_239_5_end
        if_32_26_239_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_239_5_end:
;       [239:5] free scratch register r15
    assert_239_5_end:
;   [241:5] var o3[2] object
;   [241:9] o3: object[2] (40 B @ [rbp + 464])
;   [241:9] zero 2 * 20 B = 40 B
;   [241:5] allocate named register rax
;   [241:5] allocate named register rdi
;   [241:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
;   [241:5] free named register rcx
;   [241:5] free named register rdi
;   [241:5] free named register rax
;   [242:5] o3.pos.y = 73
;   [242:16] 73
    mov qword [rbp + 472], 73
;   [243:5] # index 0 in an array can be accessed without array index
;   [245:5] assert(o3[0].pos.y == 73)
;   [245:12] allocate scratch register -> r15
;   [245:12] ? o3[0].pos.y == 73
;   [245:12] ? o3[0].pos.y == 73
    cmp_245_12:
;   [245:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [245:12] allocate scratch register -> r13
;   [245:15] set array index
;   [245:15] 0
    mov r13, 0
;   [245:15] bounds check
;   [245:15] allocate scratch register -> r12
;   [245:15] line number
    mov r12, 245
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [245:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [245:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [245:12] free scratch register r14
    sete r15b
    bool_end_245_12:
;   [32:6] assert(x bool)
    assert_245_5:
;       [245:5] alias x -> r15b
        if_32_26_245_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_245_5:
        cmp r15b, 0
        jne if_32_23_245_5_end
        if_32_26_245_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_245_5_end:
;       [245:5] free scratch register r15
    assert_245_5_end:
;   [247:5] o3[1] = object_init()
;   [247:5] allocate scratch register -> r15
    lea r15, [rbp + 464]
;   [247:5] allocate scratch register -> r14
;   [247:8] set array index
;   [247:8] 1
    mov r14, 1
;   [247:8] bounds check
;   [247:8] allocate scratch register -> r13
;   [247:8] line number
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [247:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [247:5] free scratch register r14
;   [247:13] object_init()
;   [84:6] object_init() res object
    object_init_247_13:
;       [247:13] alias res -> o3 (lea: r15)
;       [85:5] res.pos.y = 74
;       [85:17] 74
        mov qword [r15 + 8], 74
    object_init_247_13_end:
;   [247:5] free scratch register r15
;   [248:5] assert(o3[1].pos.y == 74)
;   [248:12] allocate scratch register -> r15
;   [248:12] ? o3[1].pos.y == 74
;   [248:12] ? o3[1].pos.y == 74
    cmp_248_12:
;   [248:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [248:12] allocate scratch register -> r13
;   [248:15] set array index
;   [248:15] 1
    mov r13, 1
;   [248:15] bounds check
;   [248:15] allocate scratch register -> r12
;   [248:15] line number
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [248:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [248:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [248:12] free scratch register r14
    sete r15b
    bool_end_248_12:
;   [32:6] assert(x bool)
    assert_248_5:
;       [248:5] alias x -> r15b
        if_32_26_248_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_248_5:
        cmp r15b, 0
        jne if_32_23_248_5_end
        if_32_26_248_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_248_5_end:
;       [248:5] free scratch register r15
    assert_248_5_end:
;   [250:5] var worlds[8] world
;   [250:9] worlds: world[8] (512 B @ [rbp + 504])
;   [250:9] zero 8 * 64 B = 512 B
;   [250:5] allocate named register rax
;   [250:5] allocate named register rdi
;   [250:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
;   [250:5] free named register rcx
;   [250:5] free named register rdi
;   [250:5] free named register rax
;   [251:5] worlds[1].locations[1] = 0xffee
;   [251:5] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [251:5] allocate scratch register -> r14
;   [251:12] set array index
;   [251:12] 1
    mov r14, 1
;   [251:12] bounds check
;   [251:12] allocate scratch register -> r13
;   [251:12] line number
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [251:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [251:5] free scratch register r14
;   [251:5] allocate scratch register -> r14
;   [251:25] set array index
;   [251:25] 1
    mov r14, 1
;   [251:25] bounds check
;   [251:25] allocate scratch register -> r13
;   [251:25] line number
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [251:25] free scratch register r13
;   [251:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [251:5] free scratch register r14
;   [251:5] free scratch register r15
;   [252:5] assert(worlds[1].locations[1] == 0xffee)
;   [252:12] allocate scratch register -> r15
;   [252:12] ? worlds[1].locations[1] == 0xffee
;   [252:12] ? worlds[1].locations[1] == 0xffee
    cmp_252_12:
;   [252:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [252:12] allocate scratch register -> r13
;   [252:19] set array index
;   [252:19] 1
    mov r13, 1
;   [252:19] bounds check
;   [252:19] allocate scratch register -> r12
;   [252:19] line number
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [252:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [252:12] free scratch register r13
;   [252:12] allocate scratch register -> r13
;   [252:32] set array index
;   [252:32] 1
    mov r13, 1
;   [252:32] bounds check
;   [252:32] allocate scratch register -> r12
;   [252:32] line number
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [252:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [252:12] free scratch register r13
;   [252:12] free scratch register r14
    sete r15b
    bool_end_252_12:
;   [32:6] assert(x bool)
    assert_252_5:
;       [252:5] alias x -> r15b
        if_32_26_252_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_252_5:
        cmp r15b, 0
        jne if_32_23_252_5_end
        if_32_26_252_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_252_5_end:
;       [252:5] free scratch register r15
    assert_252_5_end:
;   [254:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [254:5] allocate named register rsi
;   [254:5] allocate named register rdi
;   [254:5] allocate named register rcx
;   [257:9] array_size_of(worlds.locations)
;   [257:9] rcx = array_size_of(worlds.locations)
;   [257:9] = expression
;   [257:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [255:9] worlds[1].locations
;   [255:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [255:9] allocate scratch register -> r14
;   [255:16] set array index
;   [255:16] 1
    mov r14, 1
;   [255:16] bounds check
;   [255:16] allocate scratch register -> r13
;   [255:16] line number
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [255:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [255:9] free scratch register r14
;   [255:9] bounds check
;   [255:9] allocate scratch register -> r14
;   [255:9] line number
    mov r14, 255
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [255:9] free scratch register r14
    lea rsi, [r15]
;   [254:5] free scratch register r15
;   [256:9] worlds[0].locations
;   [256:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [256:9] allocate scratch register -> r14
;   [256:16] set array index
;   [256:16] 0
    mov r14, 0
;   [256:16] bounds check
;   [256:16] allocate scratch register -> r13
;   [256:16] line number
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
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
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [256:9] free scratch register r14
    lea rdi, [r15]
;   [254:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [254:5] free named register rcx
;   [254:5] free named register rdi
;   [254:5] free named register rsi
;   [259:5] # `array_copy` is built-in and can use indexed positions
;   [260:5] # `array_size_of` is built-in
;   [262:5] assert(worlds[0].locations[1] == 0xffee)
;   [262:12] allocate scratch register -> r15
;   [262:12] ? worlds[0].locations[1] == 0xffee
;   [262:12] ? worlds[0].locations[1] == 0xffee
    cmp_262_12:
;   [262:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [262:12] allocate scratch register -> r13
;   [262:19] set array index
;   [262:19] 0
    mov r13, 0
;   [262:19] bounds check
;   [262:19] allocate scratch register -> r12
;   [262:19] line number
    mov r12, 262
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [262:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [262:12] free scratch register r13
;   [262:12] allocate scratch register -> r13
;   [262:32] set array index
;   [262:32] 1
    mov r13, 1
;   [262:32] bounds check
;   [262:32] allocate scratch register -> r12
;   [262:32] line number
    mov r12, 262
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [262:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [262:12] free scratch register r13
;   [262:12] free scratch register r14
    sete r15b
    bool_end_262_12:
;   [32:6] assert(x bool)
    assert_262_5:
;       [262:5] alias x -> r15b
        if_32_26_262_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_262_5:
        cmp r15b, 0
        jne if_32_23_262_5_end
        if_32_26_262_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_262_5_end:
;       [262:5] free scratch register r15
    assert_262_5_end:
;   [263:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [263:12] allocate scratch register -> r15
;   [263:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [263:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_263_12:
;       [263:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [263:12] allocate named register rsi
;       [263:12] allocate named register rdi
;       [263:12] allocate named register rcx
;       [266:14] array_size_of(worlds.locations)
;       [266:14] rcx = array_size_of(worlds.locations)
;       [266:14] = expression
;       [266:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [264:14] worlds[0].locations
;       [264:14] allocate scratch register -> r14
        lea r14, [rbp + 504]
;       [264:14] allocate scratch register -> r13
;       [264:21] set array index
;       [264:21] 0
        mov r13, 0
;       [264:21] bounds check
;       [264:21] allocate scratch register -> r12
;       [264:21] line number
        mov r12, 264
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
;       [264:21] free scratch register r12
        shl r13, 6
        add r14, r13
;       [264:14] free scratch register r13
;       [264:14] bounds check
;       [264:14] allocate scratch register -> r13
;       [264:14] line number
        mov r13, 264
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [264:14] free scratch register r13
        lea rsi, [r14]
;       [263:12] free scratch register r14
;       [265:14] worlds[1].locations
;       [265:14] allocate scratch register -> r14
        lea r14, [rbp + 504]
;       [265:14] allocate scratch register -> r13
;       [265:21] set array index
;       [265:21] 1
        mov r13, 1
;       [265:21] bounds check
;       [265:21] allocate scratch register -> r12
;       [265:21] line number
        mov r12, 265
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
;       [265:21] free scratch register r12
        shl r13, 6
        add r14, r13
;       [265:14] free scratch register r13
;       [265:14] bounds check
;       [265:14] allocate scratch register -> r13
;       [265:14] line number
        mov r13, 265
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [265:14] free scratch register r13
        lea rdi, [r14]
;       [263:12] free scratch register r14
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [263:12] free named register rcx
;       [263:12] free named register rdi
;       [263:12] free named register rsi
        sete r15b
    bool_end_263_12:
;   [32:6] assert(x bool)
    assert_263_5:
;       [263:5] alias x -> r15b
        if_32_26_263_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_263_5:
        cmp r15b, 0
        jne if_32_23_263_5_end
        if_32_26_263_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_263_5_end:
;       [263:5] free scratch register r15
    assert_263_5_end:
;   [269:5] var arr2[] = { -1, 2 }
;   [269:9] arr2: i64[2] (16 B @ [rbp + 1016])
;   [269:9] arr2= { -1, 2 }
;   [269:20] [0]
;   [269:21] -1
    mov qword [rbp + 1016], -1
;   [269:20] [1]
;   [269:24] 2
    mov qword [rbp + 1024], 2
;   [270:5] assert(array_size_of(arr2) == 2)
;   [270:12] allocate scratch register -> r15
;   [270:12] ? array_size_of(arr2) == 2
;   [270:12] ? array_size_of(arr2) == 2
    cmp_270_12:
;   [270:12] allocate scratch register -> r14
;       [270:12] r14 = array_size_of(arr2)
;       [270:12] = expression
;       [270:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [270:12] free scratch register r14
    sete r15b
    bool_end_270_12:
;   [32:6] assert(x bool)
    assert_270_5:
;       [270:5] alias x -> r15b
        if_32_26_270_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_270_5:
        cmp r15b, 0
        jne if_32_23_270_5_end
        if_32_26_270_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_270_5_end:
;       [270:5] free scratch register r15
    assert_270_5_end:
;   [271:5] assert(arr2[0] == -1)
;   [271:12] allocate scratch register -> r15
;   [271:12] ? arr2[0] == -1
;   [271:12] ? arr2[0] == -1
    cmp_271_12:
;   [271:12] allocate scratch register -> r14
;   [271:17] set array index
;   [271:17] 0
    mov r14, 0
;   [271:17] bounds check
;   [271:17] allocate scratch register -> r13
;   [271:17] line number
    mov r13, 271
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [271:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], -1
;   [271:12] free scratch register r14
    sete r15b
    bool_end_271_12:
;   [32:6] assert(x bool)
    assert_271_5:
;       [271:5] alias x -> r15b
        if_32_26_271_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_271_5:
        cmp r15b, 0
        jne if_32_23_271_5_end
        if_32_26_271_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_271_5_end:
;       [271:5] free scratch register r15
    assert_271_5_end:
;   [272:5] assert(arr2[1] == 2)
;   [272:12] allocate scratch register -> r15
;   [272:12] ? arr2[1] == 2
;   [272:12] ? arr2[1] == 2
    cmp_272_12:
;   [272:12] allocate scratch register -> r14
;   [272:17] set array index
;   [272:17] 1
    mov r14, 1
;   [272:17] bounds check
;   [272:17] allocate scratch register -> r13
;   [272:17] line number
    mov r13, 272
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [272:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], 2
;   [272:12] free scratch register r14
    sete r15b
    bool_end_272_12:
;   [32:6] assert(x bool)
    assert_272_5:
;       [272:5] alias x -> r15b
        if_32_26_272_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_272_5:
        cmp r15b, 0
        jne if_32_23_272_5_end
        if_32_26_272_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_272_5_end:
;       [272:5] free scratch register r15
    assert_272_5_end:
;   [274:5] var counter
;   [274:9] counter: i64 (8 B @ [rbp + 1032])
;   [274:9] zero 1 * 8 B = 8 B
;   [274:5] size <= 32 B, use mov
    mov qword [rbp + 1032], 0
;   [275:5] var nm str
;   [275:9] nm: str (128 B @ [rbp + 1040])
;   [275:9] zero 1 * 128 B = 128 B
;   [275:5] allocate named register rax
;   [275:5] allocate named register rdi
;   [275:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
;   [275:5] free named register rcx
;   [275:5] free named register rdi
;   [275:5] free named register rax
;   [276:5] print(hello)
;   [34:6] print(str[] i8)
    print_276_5:
;       [276:5] alias str -> hello
;       [35:5] write(1, address_of(str), array_size_of(str))
;       [35:5] allocate named register rdi
;       [35:11] 1
        mov rdi, 1
;       [35:5] allocate named register rsi
;       [35:14] rsi = address_of(str)
;       [35:14] = expression
;       [35:14] address_of(str)
        lea rsi, [rbp]
;       [35:5] allocate named register rdx
;       [35:31] rdx = array_size_of(str)
;       [35:31] = expression
;       [35:31] array_size_of(str)
        mov rdx, 21
;       [35:5] allocate named register rax
        mov rax, 1
        syscall
;       [35:5] free named register rax
;       [35:5] free named register rdx
;       [35:5] free named register rsi
;       [35:5] free named register rdi
    print_276_5_end:
;   [277:5] label
    loop_277_5:
;       [278:9] counter = counter + 1
;       [278:19] instructions without scratch register 1, with 3
;       [278:19] counter
;       [278:29] counter + 1
        add qword [rbp + 1032], 1
;       [279:9] print_num(counter)
;       [279:9] address of argument 'counter' to parameter 'num'
;       [279:9] allocate scratch register -> r15
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
;       [279:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
;       [280:9] print(colon)
;       [34:6] print(str[] i8)
        print_280_9:
;           [280:9] alias str -> colon
;           [35:5] write(1, address_of(str), array_size_of(str))
;           [35:5] allocate named register rdi
;           [35:11] 1
            mov rdi, 1
;           [35:5] allocate named register rsi
;           [35:14] rsi = address_of(str)
;           [35:14] = expression
;           [35:14] address_of(str)
            lea rsi, [rbp + 61]
;           [35:5] allocate named register rdx
;           [35:31] rdx = array_size_of(str)
;           [35:31] = expression
;           [35:31] array_size_of(str)
            mov rdx, 2
;           [35:5] allocate named register rax
            mov rax, 1
            syscall
;           [35:5] free named register rax
;           [35:5] free named register rdx
;           [35:5] free named register rsi
;           [35:5] free named register rdi
        print_280_9_end:
;       [281:9] print(prompt1)
;       [34:6] print(str[] i8)
        print_281_9:
;           [281:9] alias str -> prompt1
;           [35:5] write(1, address_of(str), array_size_of(str))
;           [35:5] allocate named register rdi
;           [35:11] 1
            mov rdi, 1
;           [35:5] allocate named register rsi
;           [35:14] rsi = address_of(str)
;           [35:14] = expression
;           [35:14] address_of(str)
            lea rsi, [rbp + 21]
;           [35:5] allocate named register rdx
;           [35:31] rdx = array_size_of(str)
;           [35:31] = expression
;           [35:31] array_size_of(str)
            mov rdx, 12
;           [35:5] allocate named register rax
            mov rax, 1
            syscall
;           [35:5] free named register rax
;           [35:5] free named register rdx
;           [35:5] free named register rsi
;           [35:5] free named register rdi
        print_281_9_end:
;       [282:9] str_in(nm)
;       [70:6] str_in(s str)
        str_in_282_9:
;           [282:9] alias s -> nm
;           [71:5] var nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [71:9] nbytes: i64 (8 B @ [rbp + 1168])
;           [71:9] nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [71:18] nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [71:18] = expression
;           [71:18] read(0, address_of(s.data), array_size_of(s.data))
;           [71:18] allocate named register rdi
;           [71:23] 0
            mov rdi, 0
;           [71:18] allocate named register rsi
;           [71:26] rsi = address_of(s.data)
;           [71:26] = expression
;           [71:26] address_of(s.data)
            lea rsi, [rbp + 1041]
;           [71:18] allocate named register rdx
;           [71:46] rdx = array_size_of(s.data)
;           [71:46] = expression
;           [71:46] array_size_of(s.data)
            mov rdx, 127
;           [71:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
;           [71:18] free named register rax
;           [71:18] free named register rdx
;           [71:18] free named register rsi
;           [71:18] free named register rdi
;           [72:5] s.len = nbytes - 1
;           [72:13] instructions without scratch register 3, with 3
;           [72:13] nbytes
;           [72:13] allocate scratch register -> r15
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
;           [72:13] free scratch register r15
;           [72:22] s.len - 1
            sub byte [rbp + 1040], 1
        str_in_282_9_end:
        if_283_12:
;       [283:12] ? nm.len == 0
;       [283:12] ? nm.len == 0
        cmp_283_12:
        cmp byte [rbp + 1040], 0
        jne if_285_19
        if_283_12_code:
;           [284:13] break
            jmp loop_277_5_end
        jmp if_283_9_end
        if_285_19:
;       [285:19] ? nm.len <= 4
;       [285:19] ? nm.len <= 4
        cmp_285_19:
        cmp byte [rbp + 1040], 4
        jg if_else_283_9
        if_285_19_code:
;           [286:13] print(prompt2)
;           [34:6] print(str[] i8)
            print_286_13:
;               [286:13] alias str -> prompt2
;               [35:5] write(1, address_of(str), array_size_of(str))
;               [35:5] allocate named register rdi
;               [35:11] 1
                mov rdi, 1
;               [35:5] allocate named register rsi
;               [35:14] rsi = address_of(str)
;               [35:14] = expression
;               [35:14] address_of(str)
                lea rsi, [rbp + 33]
;               [35:5] allocate named register rdx
;               [35:31] rdx = array_size_of(str)
;               [35:31] = expression
;               [35:31] array_size_of(str)
                mov rdx, 20
;               [35:5] allocate named register rax
                mov rax, 1
                syscall
;               [35:5] free named register rax
;               [35:5] free named register rdx
;               [35:5] free named register rsi
;               [35:5] free named register rdi
            print_286_13_end:
;           [287:13] continue
            jmp loop_277_5
        jmp if_283_9_end
        if_else_283_9:
;           [289:13] print(prompt3)
;           [34:6] print(str[] i8)
            print_289_13:
;               [289:13] alias str -> prompt3
;               [35:5] write(1, address_of(str), array_size_of(str))
;               [35:5] allocate named register rdi
;               [35:11] 1
                mov rdi, 1
;               [35:5] allocate named register rsi
;               [35:14] rsi = address_of(str)
;               [35:14] = expression
;               [35:14] address_of(str)
                lea rsi, [rbp + 53]
;               [35:5] allocate named register rdx
;               [35:31] rdx = array_size_of(str)
;               [35:31] = expression
;               [35:31] array_size_of(str)
                mov rdx, 6
;               [35:5] allocate named register rax
                mov rax, 1
                syscall
;               [35:5] free named register rax
;               [35:5] free named register rdx
;               [35:5] free named register rsi
;               [35:5] free named register rdi
            print_289_13_end:
;           [290:13] str_out(nm)
;           [75:6] str_out(s str)
            str_out_290_13:
;               [290:13] alias s -> nm
;               [76:5] write(1, address_of(s.data), s.len)
;               [76:5] allocate named register rdi
;               [76:11] 1
                mov rdi, 1
;               [76:5] allocate named register rsi
;               [76:14] rsi = address_of(s.data)
;               [76:14] = expression
;               [76:14] address_of(s.data)
                lea rsi, [rbp + 1041]
;               [76:5] allocate named register rdx
;               [76:34] s.len
                movsx rdx, byte [rbp + 1040]
;               [76:5] allocate named register rax
                mov rax, 1
                syscall
;               [76:5] free named register rax
;               [76:5] free named register rdx
;               [76:5] free named register rsi
;               [76:5] free named register rdi
            str_out_290_13_end:
;           [291:13] print(dot)
;           [34:6] print(str[] i8)
            print_291_13:
;               [291:13] alias str -> dot
;               [35:5] write(1, address_of(str), array_size_of(str))
;               [35:5] allocate named register rdi
;               [35:11] 1
                mov rdi, 1
;               [35:5] allocate named register rsi
;               [35:14] rsi = address_of(str)
;               [35:14] = expression
;               [35:14] address_of(str)
                lea rsi, [rbp + 59]
;               [35:5] allocate named register rdx
;               [35:31] rdx = array_size_of(str)
;               [35:31] = expression
;               [35:31] array_size_of(str)
                mov rdx, 1
;               [35:5] allocate named register rax
                mov rax, 1
                syscall
;               [35:5] free named register rax
;               [35:5] free named register rdx
;               [35:5] free named register rsi
;               [35:5] free named register rdi
            print_291_13_end:
;           [292:13] print(nl)
;           [34:6] print(str[] i8)
            print_292_13:
;               [292:13] alias str -> nl
;               [35:5] write(1, address_of(str), array_size_of(str))
;               [35:5] allocate named register rdi
;               [35:11] 1
                mov rdi, 1
;               [35:5] allocate named register rsi
;               [35:14] rsi = address_of(str)
;               [35:14] = expression
;               [35:14] address_of(str)
                lea rsi, [rbp + 60]
;               [35:5] allocate named register rdx
;               [35:31] rdx = array_size_of(str)
;               [35:31] = expression
;               [35:31] array_size_of(str)
                mov rdx, 1
;               [35:5] allocate named register rax
                mov rax, 1
                syscall
;               [35:5] free named register rax
;               [35:5] free named register rdx
;               [35:5] free named register rsi
;               [35:5] free named register rdi
            print_292_13_end:
        if_283_9_end:
    jmp loop_277_5
    loop_277_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; 
;[96:15] noinline print_num(num)
print_num:
;   [96:25] num: i64 (8 B @ [rbx])
;   [97:5] var buf[20] i8
;   [97:9] buf: i8[20] (20 B @ [rbx + 8])
;   [97:9] zero 20 * 1 B = 20 B
;   [97:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [98:5] var n = num
;   [98:9] n: i64 (8 B @ [rbx + 28])
;   [98:9] n = num
;   [98:13] num
;   [98:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [98:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
;   [98:13] free scratch register r14
;   [98:13] free scratch register r15
;   [99:5] var is_negative bool = false
;   [99:9] is_negative: bool (1 B @ [rbx + 36])
;   [99:9] is_negative = false
    mov byte [rbx + 36], 0
    if_101_8:
;   [101:8] ? n < 0
;   [101:8] ? n < 0
    cmp_101_8:
    cmp qword [rbx + 28], 0
    jge if_101_5_end
    if_101_8_code:
;       [102:9] is_negative = true
        mov byte [rbx + 36], 1
;       [103:9] n = -n
;       [103:14] -n
        neg qword [rbx + 28]
    if_101_5_end:
;   [106:5] var i = 20
;   [106:9] i: i64 (8 B @ [rbx + 37])
;   [106:9] i = 20
;   [106:13] 20
    mov qword [rbx + 37], 20
;   [107:5] label
    loop_107_5:
;       [108:9] i = i - 1
;       [108:13] instructions without scratch register 1, with 3
;       [108:13] i
;       [108:17] i - 1
        sub qword [rbx + 37], 1
;       [109:9] var ascii = 48 + (n % 10)
;       [109:13] ascii: i64 (8 B @ [rbx + 45])
;       [109:13] ascii = 48 + (n % 10)
;       [109:21] instructions without scratch register 8, with 9
;       [109:21] 48
        mov qword [rbx + 45], 48
;       [109:27] ascii + (n % 10)
;       [109:27] allocate scratch register -> r15
;       [109:27] n
        mov r15, qword [rbx + 28]
;       [109:31] r15 % 10
;       [109:31] div const
;       [109:31] allocate named register rax
        mov rax, r15
;       [109:31] allocate named register rdx
        cqo
;       [109:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [109:31] free scratch register r14
        mov r15, rdx
;       [109:31] free named register rdx
;       [109:31] free named register rax
        add qword [rbx + 45], r15
;       [109:27] free scratch register r15
;       [110:9] # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
;       [111:9] buf[i] = ascii
;       [111:9] allocate scratch register -> r15
;       [111:13] set array index
;       [111:13] i
        mov r15, qword [rbx + 37]
;       [111:13] bounds check
;       [111:13] allocate scratch register -> r14
;       [111:13] line number
        mov r14, 111
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [111:13] free scratch register r14
;       [111:18] ascii
;       [111:18] allocate scratch register -> r14
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
;       [111:18] free scratch register r14
;       [111:9] free scratch register r15
;       [112:9] n = n / 10
;       [112:13] instructions without scratch register 5, with 7
;       [112:13] n
;       [112:17] n / 10
;       [112:17] div const
;       [112:17] allocate named register rax
        mov rax, qword [rbx + 28]
;       [112:17] allocate named register rdx
        cqo
;       [112:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [112:17] free scratch register r15
        mov qword [rbx + 28], rax
;       [112:17] free named register rdx
;       [112:17] free named register rax
        if_113_12:
;       [113:12] ? n == 0
;       [113:12] ? n == 0
        cmp_113_12:
        cmp qword [rbx + 28], 0
        jne if_113_9_end
        if_113_12_code:
;           [113:19] break
            jmp loop_107_5_end
        if_113_9_end:
    jmp loop_107_5
    loop_107_5_end:
    if_116_8:
;   [116:8] ? is_negative
;   [116:8] ? is_negative
    cmp_116_8:
    cmp byte [rbx + 36], 0
    je if_116_5_end
    if_116_8_code:
;       [117:9] i = i - 1
;       [117:13] instructions without scratch register 1, with 3
;       [117:13] i
;       [117:17] i - 1
        sub qword [rbx + 37], 1
;       [118:9] buf[i] = 45
;       [118:9] allocate scratch register -> r15
;       [118:13] set array index
;       [118:13] i
        mov r15, qword [rbx + 37]
;       [118:13] bounds check
;       [118:13] allocate scratch register -> r14
;       [118:13] line number
        mov r14, 118
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [118:13] free scratch register r14
;       [118:18] 45
        mov byte [rbx + r15 + 8], 45
;       [118:9] free scratch register r15
    if_116_5_end:
;   [121:5] var write_pos = 0
;   [121:9] write_pos: i64 (8 B @ [rbx + 45])
;   [121:9] write_pos = 0
;   [121:21] 0
    mov qword [rbx + 45], 0
;   [122:5] label
    loop_122_5:
;       [123:9] buf[write_pos] = buf[i]
;       [123:9] allocate scratch register -> r15
;       [123:13] set array index
;       [123:13] write_pos
        mov r15, qword [rbx + 45]
;       [123:13] bounds check
;       [123:13] allocate scratch register -> r14
;       [123:13] line number
        mov r14, 123
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [123:13] free scratch register r14
;       [123:26] buf[i]
;       [123:26] allocate scratch register -> r14
;       [123:30] set array index
;       [123:30] i
        mov r14, qword [rbx + 37]
;       [123:30] bounds check
;       [123:30] allocate scratch register -> r13
;       [123:30] line number
        mov r13, 123
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [123:30] free scratch register r13
;       [123:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [123:26] free scratch register r13
;       [123:26] free scratch register r14
;       [123:9] free scratch register r15
;       [124:9] write_pos = write_pos + 1
;       [124:21] instructions without scratch register 1, with 3
;       [124:21] write_pos
;       [124:33] write_pos + 1
        add qword [rbx + 45], 1
;       [125:9] i = i + 1
;       [125:13] instructions without scratch register 1, with 3
;       [125:13] i
;       [125:17] i + 1
        add qword [rbx + 37], 1
        if_126_12:
;       [126:12] ? i == 20
;       [126:12] ? i == 20
        cmp_126_12:
        cmp qword [rbx + 37], 20
        jne if_126_9_end
        if_126_12_code:
;           [126:20] break
            jmp loop_122_5_end
        if_126_9_end:
    jmp loop_122_5
    loop_122_5_end:
;   [129:5] write(1, address_of(buf), write_pos)
;   [129:5] allocate named register rdi
;   [129:11] 1
    mov rdi, 1
;   [129:5] allocate named register rsi
;   [129:14] rsi = address_of(buf)
;   [129:14] = expression
;   [129:14] address_of(buf)
    lea rsi, [rbx + 8]
;   [129:5] allocate named register rdx
;   [129:31] write_pos
    mov rdx, qword [rbx + 45]
;   [129:5] allocate named register rax
    mov rax, 1
    syscall
;   [129:5] free named register rax
;   [129:5] free named register rdx
;   [129:5] free named register rsi
;   [129:5] free named register rdi
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
;[20:20] i8[21]
db `hello world from baz\n`
;[21:5] prompt1
;[21:20] i8[12]
db `enter name:\n`
;[22:5] prompt2
;[22:20] i8[20]
db `that is not a name.\n`
;[23:5] prompt3
;[23:20] i8[6]
db `hello `
;[24:9] dot
;[24:20] i8[1]
db `.`
;[25:10] nl
;[25:20] i8[1]
db `\n`
;[26:7] colon
;[26:20] i8[2]
db `: `
;[27:8] nums
; i64[4]
;[27:20] [0]
;[27:20] i64
dq 1
; pad 3 'i64' of size 8
times 24 db 0
;[28:8] str1
;[28:21] i8
db 3
;[28:19] zero remaining fields
times 127 db 0
dat.end:

section .bss.vars nobits alloc write
align 16
vars:
vars resb 131072
vars.end:
; free named register rbp

; max scratch registers in use: 4
;            max frames in use: 8
;              dat var padding: 1 B
;                max vars size: 952 B
;          optimization pass 1: 119
;          optimization pass 2: 0
```
