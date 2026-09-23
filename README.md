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
C/C++ Header                    52           3224            994          11820
C++                              1             46              7            235
-------------------------------------------------------------------------------
SUM:                            53           3270           1001          12055
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
# exit is a built-in function

func print(str[] i8) {
    write(1, address_of(str), array_size_of(str))
    # write is a built-in functions writes to a file descriptor
}

# function arguments and return are equivalent to mutable references

func fooz(pt point) {
    pt.x = 0b10    # binary value 2
    pt.y = 0xb     # hex value 11
}

# default argument type is i64 on x86_64 and i32 on rv32i
# arguments are references to memory locations

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}

# return is a reference to the target with optional type
# it is accessed as a variable, in this case `res`

func inv(i i32) res i32 {
    res = ~i
}

func baz(arg) res {
    res = arg * 2
}

# array arguments are declared with [] and optional type

func faz(arg[] i32) {
    arg[1] = 0xfe
}

func str_in(s str) {
    var nbytes = read(0, address_of(s.data), array_size_of(s.data))
    # read is built-in function that operates on file descriptors
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
# arguments and return are references to memory locations
# arrays not supported

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
        # note: not buf[i] = 48 + ... because expression will be executed as
        #       byte sized and n overflows
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
        # a code block opens a new scope
        # constants and variables shadow outer scope
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
    assert(arr[0] == 2)
    # `array_copy` is a built-in function: copy from, to, number of elements

    var arr1[8] i32
    array_copy(arr, arr1, 4)
    var eq bool = arrays_equal(arr[1], arr1[1], 3)
    # type `bool` is built-in
    # `arrays_equal` is built-in function comparing source and destination
    assert(eq)

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
    cmp_144_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_144_12:
    assert_144_5:
        if_32_26_144_5:
        cmp_32_26_144_5:
        cmp r15b, 0
        jne if_32_23_144_5_end
        if_32_26_144_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_144_5_end:
    assert_144_5_end:
    mov qword [rbp + 240], -1
    cmp_148_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_148_12:
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
        cmp_154_16:
        bool_end_154_16:
        mov r15b, 1
        assert_154_9:
            if_32_26_154_9:
            cmp_32_26_154_9:
            cmp r15b, 0
            jne if_32_23_154_9_end
            if_32_26_154_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_32_23_154_9_end:
        assert_154_9_end:
    cmp_157_12:
    bool_end_157_12:
    mov r15b, 1
    assert_157_5:
        if_32_26_157_5:
        cmp_32_26_157_5:
        cmp r15b, 0
        jne if_32_23_157_5_end
        if_32_26_157_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_157_5_end:
    assert_157_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_164_12:
    mov r14, 1
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
    cmp_165_12:
    mov r14, 2
    mov r13, 165
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_165_12:
    assert_165_5:
        if_32_26_165_5:
        cmp_32_26_165_5:
        cmp r15b, 0
        jne if_32_23_165_5_end
        if_32_26_165_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_165_5_end:
    assert_165_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_168_12:
    mov r14, 0
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
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
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 172
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 172
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_173_19:
        mov rcx, 3
        mov r15, 1
        mov r14, 173
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 224]
        mov r15, 1
        mov r14, 173
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 288]
    bool_end_173_19:
    cmp_176_12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool_end_176_12:
    assert_176_5:
        if_32_26_176_5:
        cmp_32_26_176_5:
        cmp r15b, 0
        jne if_32_23_176_5_end
        if_32_26_176_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_176_5_end:
    assert_176_5_end:
    mov r15, 2
    mov r14, 178
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_179_12:
        mov rcx, 4
        mov r14, 179
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 179
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
    bool_end_179_12:
    assert_179_5:
        if_32_26_179_5:
        cmp_32_26_179_5:
        cmp r15b, 0
        jne if_32_23_179_5_end
        if_32_26_179_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_179_5_end:
    assert_179_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_182_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_182_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_183_12:
    mov r14, qword [rbp + 248]
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_32_26_183_5:
        cmp_32_26_183_5:
        cmp r15b, 0
        jne if_32_23_183_5_end
        if_32_26_183_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_183_5_end:
    assert_183_5_end:
    faz_185_5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_185_5_end:
    cmp_186_12:
    mov r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_32_26_186_5:
        cmp_32_26_186_5:
        cmp r15b, 0
        jne if_32_23_186_5_end
        if_32_26_186_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_186_5_end:
    assert_186_5_end:
    mov qword [rbp + 289], 3
    mov qword [rbp + 297], 5
    lea r15, [rbp + 289]
    mov qword [rbp + 313], 0
    foo_189_5:
        mov r14, qword [rbp + 313]
        add qword [r15], r14
        add qword [r15], 2
        foo_189_5_continue:
            add r15, 8
            inc qword [rbp + 313]
            cmp qword [rbp + 313], 2
            jne foo_189_5
    foo_189_5_end:
    cmp_192_12:
    mov r14, 0
    mov r13, 192
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_32_26_192_5:
        cmp_32_26_192_5:
        cmp r15b, 0
        jne if_32_23_192_5_end
        if_32_26_192_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_192_5_end:
    assert_192_5_end:
    cmp_193_12:
    mov r14, 1
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
    sete r15b
    bool_end_193_12:
    assert_193_5:
        if_32_26_193_5:
        cmp_32_26_193_5:
        cmp r15b, 0
        jne if_32_23_193_5_end
        if_32_26_193_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_193_5_end:
    assert_193_5_end:
    mov qword [rbp + 305], 0
    mov qword [rbp + 313], 0
    fooz_200_5:
        mov qword [rbp + 305], 2
        mov qword [rbp + 313], 11
    fooz_200_5_end:
    cmp_201_12:
    cmp qword [rbp + 305], 2
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_32_26_201_5:
        cmp_32_26_201_5:
        cmp r15b, 0
        jne if_32_23_201_5_end
        if_32_26_201_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_201_5_end:
    assert_201_5_end:
    cmp_202_12:
    cmp qword [rbp + 313], 11
    sete r15b
    bool_end_202_12:
    assert_202_5:
        if_32_26_202_5:
        cmp_32_26_202_5:
        cmp r15b, 0
        jne if_32_23_202_5_end
        if_32_26_202_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_202_5_end:
    assert_202_5_end:
    mov rax, qword [rbp + 305]
    mov qword [rbp + 321], rax
    mov rax, qword [rbp + 313]
    mov qword [rbp + 329], rax
    cmp_205_12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool_end_205_12:
    assert_205_5:
        if_32_26_205_5:
        cmp_32_26_205_5:
        cmp r15b, 0
        jne if_32_23_205_5_end
        if_32_26_205_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_205_5_end:
    assert_205_5_end:
    mov qword [rbp + 321], 3
    cmp_210_12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_210_12:
    assert_210_5:
        if_32_26_210_5:
        cmp_32_26_210_5:
        cmp r15b, 0
        jne if_32_23_210_5_end
        if_32_26_210_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_210_5_end:
    assert_210_5_end:
    mov qword [rbp + 337], 0
    bar_213_5:
        if_51_8_213_5:
        cmp_51_8_213_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_213_5_end
        if_51_8_213_5_code:
            jmp bar_213_5_end
        if_51_5_213_5_end:
        mov qword [rbp + 337], 255
    bar_213_5_end:
    cmp_214_12:
    cmp qword [rbp + 337], 0
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
    mov qword [rbp + 337], 1
    bar_217_5:
        if_51_8_217_5:
        cmp_51_8_217_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_217_5_end
        if_51_8_217_5_code:
            jmp bar_217_5_end
        if_51_5_217_5_end:
        mov qword [rbp + 337], 255
    bar_217_5_end:
    cmp_218_12:
    cmp qword [rbp + 337], 255
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_32_26_218_5:
        cmp_32_26_218_5:
        cmp r15b, 0
        jne if_32_23_218_5_end
        if_32_26_218_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_218_5_end:
    assert_218_5_end:
    mov qword [rbp + 345], 1
    baz_221_13:
        mov r15, qword [rbp + 345]
        imul r15, 2
        mov qword [rbp + 353], r15
    baz_221_13_end:
    cmp_222_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_32_26_222_5:
        cmp_32_26_222_5:
        cmp r15b, 0
        jne if_32_23_222_5_end
        if_32_26_222_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_222_5_end:
    assert_222_5_end:
    baz_224_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 353], r15
    baz_224_9_end:
    cmp_225_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_32_26_225_5:
        cmp_32_26_225_5:
        cmp r15b, 0
        jne if_32_23_225_5_end
        if_32_26_225_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_225_5_end:
    assert_225_5_end:
    baz_227_21:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 361], r15
    baz_227_21_end:
    mov qword [rbp + 369], 0
    cmp_228_12:
    cmp qword [rbp + 361], 6
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
    point_init_230_20:
        mov qword [rbp + 377], -1
        mov qword [rbp + 385], -2
    point_init_230_20_end:
    cmp_231_12:
    cmp qword [rbp + 377], -1
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_32_26_231_5:
        cmp_32_26_231_5:
        cmp r15b, 0
        jne if_32_23_231_5_end
        if_32_26_231_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_231_5_end:
    assert_231_5_end:
    cmp_232_12:
    cmp qword [rbp + 385], -2
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_32_26_232_5:
        cmp_32_26_232_5:
        cmp r15b, 0
        jne if_32_23_232_5_end
        if_32_26_232_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_232_5_end:
    assert_232_5_end:
    mov qword [rbp + 393], 1
    mov qword [rbp + 401], 2
    mov r15, qword [rbp + 393]
    imul r15, 10
    mov qword [rbp + 409], r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 417], r15
    mov dword [rbp + 425], 16711680
    cmp_238_12:
    cmp qword [rbp + 409], 10
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
    cmp qword [rbp + 417], 2
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
    cmp_240_12:
    cmp dword [rbp + 425], 16711680
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_32_26_240_5:
        cmp_32_26_240_5:
        cmp r15b, 0
        jne if_32_23_240_5_end
        if_32_26_240_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_240_5_end:
    assert_240_5_end:
    mov r15, qword [rbp + 393]
    mov qword [rbp + 429], r15
    neg qword [rbp + 429]
    mov r15, qword [rbp + 401]
    mov qword [rbp + 437], r15
    neg qword [rbp + 437]
    mov rax, qword [rbp + 429]
    mov qword [rbp + 409], rax
    mov rax, qword [rbp + 437]
    mov qword [rbp + 417], rax
    cmp_244_12:
    cmp qword [rbp + 409], -1
    sete r15b
    bool_end_244_12:
    assert_244_5:
        if_32_26_244_5:
        cmp_32_26_244_5:
        cmp r15b, 0
        jne if_32_23_244_5_end
        if_32_26_244_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_244_5_end:
    assert_244_5_end:
    cmp_245_12:
    cmp qword [rbp + 417], -2
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
    lea rsi, [rbp + 409]
    lea rdi, [rbp + 445]
    mov rcx, 20
    rep movsb
    cmp_248_12:
    cmp qword [rbp + 445], -1
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
    cmp_249_12:
    cmp qword [rbp + 453], -2
    sete r15b
    bool_end_249_12:
    assert_249_5:
        if_32_26_249_5:
        cmp_32_26_249_5:
        cmp r15b, 0
        jne if_32_23_249_5_end
        if_32_26_249_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_249_5_end:
    assert_249_5_end:
    cmp_250_12:
    cmp dword [rbp + 461], 16711680
    sete r15b
    bool_end_250_12:
    assert_250_5:
        if_32_26_250_5:
        cmp_32_26_250_5:
        cmp r15b, 0
        jne if_32_23_250_5_end
        if_32_26_250_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_250_5_end:
    assert_250_5_end:
    xor al, al
    lea rdi, [rbp + 465]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 473], 73
    cmp_256_12:
    lea r14, [rbp + 465]
    mov r13, 0
    mov r12, 256
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
    bool_end_256_12:
    assert_256_5:
        if_32_26_256_5:
        cmp_32_26_256_5:
        cmp r15b, 0
        jne if_32_23_256_5_end
        if_32_26_256_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_256_5_end:
    assert_256_5_end:
    lea r15, [rbp + 465]
    mov r14, 1
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_258_13:
        mov qword [r15 + 8], 74
    object_init_258_13_end:
    cmp_259_12:
    lea r14, [rbp + 465]
    mov r13, 1
    mov r12, 259
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
    bool_end_259_12:
    assert_259_5:
        if_32_26_259_5:
        cmp_32_26_259_5:
        cmp r15b, 0
        jne if_32_23_259_5_end
        if_32_26_259_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_259_5_end:
    assert_259_5_end:
    xor al, al
    lea rdi, [rbp + 505]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 505]
    mov r14, 1
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_263_12:
    lea r14, [rbp + 505]
    mov r13, 1
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    lea r15, [rbp + 505]
    mov r14, 1
    mov r13, 266
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 266
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 505]
    mov r14, 0
    mov r13, 267
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 267
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_272_12:
    lea r14, [rbp + 505]
    mov r13, 0
    mov r12, 272
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 272
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    cmp_273_12:
        mov rcx, 8
        lea r14, [rbp + 505]
        mov r13, 0
        mov r12, 274
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 274
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [r14]
        lea r14, [rbp + 505]
        mov r13, 1
        mov r12, 275
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 275
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
    bool_end_273_12:
    assert_273_5:
        if_32_26_273_5:
        cmp_32_26_273_5:
        cmp r15b, 0
        jne if_32_23_273_5_end
        if_32_26_273_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_273_5_end:
    assert_273_5_end:
    mov qword [rbp + 1017], -1
    mov qword [rbp + 1025], 2
    cmp_280_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_280_12:
    assert_280_5:
        if_32_26_280_5:
        cmp_32_26_280_5:
        cmp r15b, 0
        jne if_32_23_280_5_end
        if_32_26_280_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_280_5_end:
    assert_280_5_end:
    cmp_281_12:
    mov r14, 0
    mov r13, 281
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], -1
    sete r15b
    bool_end_281_12:
    assert_281_5:
        if_32_26_281_5:
        cmp_32_26_281_5:
        cmp r15b, 0
        jne if_32_23_281_5_end
        if_32_26_281_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_281_5_end:
    assert_281_5_end:
    cmp_282_12:
    mov r14, 1
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], 2
    sete r15b
    bool_end_282_12:
    assert_282_5:
        if_32_26_282_5:
        cmp_32_26_282_5:
        cmp r15b, 0
        jne if_32_23_282_5_end
        if_32_26_282_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_282_5_end:
    assert_282_5_end:
    mov qword [rbp + 1033], 0
    xor al, al
    lea rdi, [rbp + 1041]
    mov rcx, 128
    rep stosb
    print_286_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_286_5_end:
    loop_287_5:
        add qword [rbp + 1033], 1
        lea r15, [rbp + 1033]
        mov qword [rbp + 1169], r15
        PUSH_REGS
        lea rbx, [rbp + 1169]
        call print_num
        POP_REGS
        print_290_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_290_9_end:
        print_291_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_291_9_end:
        str_in_292_9:
            mov rdi, 0
            lea rsi, [rbp + 1042]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1169], rax
            mov r15b, byte [rbp + 1169]
            mov byte [rbp + 1041], r15b
            sub byte [rbp + 1041], 1
        str_in_292_9_end:
        if_293_12:
        cmp_293_12:
        cmp byte [rbp + 1041], 0
        jne if_295_19
        if_293_12_code:
            jmp loop_287_5_end
        jmp if_293_9_end
        if_295_19:
        cmp_295_19:
        cmp byte [rbp + 1041], 4
        jg if_else_293_9
        if_295_19_code:
            print_296_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_296_13_end:
            jmp loop_287_5
        jmp if_293_9_end
        if_else_293_9:
            print_299_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_299_13_end:
            str_out_300_13:
                mov rdi, 1
                lea rsi, [rbp + 1042]
                movsx rdx, byte [rbp + 1041]
                mov rax, 1
                syscall
            str_out_300_13_end:
            print_301_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_301_13_end:
            print_302_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_302_13_end:
        if_293_9_end:
    jmp loop_287_5
    loop_287_5_end:
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
    if_107_8:
    cmp_107_8:
    cmp qword [rbx + 28], 0
    jge if_107_5_end
    if_107_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_107_5_end:
    mov qword [rbx + 37], 20
    loop_113_5:
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
        mov r14, 118
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
        if_120_12:
        cmp_120_12:
        cmp qword [rbx + 28], 0
        jne if_120_9_end
        if_120_12_code:
            jmp loop_113_5_end
        if_120_9_end:
    jmp loop_113_5
    loop_113_5_end:
    if_123_8:
    cmp_123_8:
    cmp byte [rbx + 36], 0
    je if_123_5_end
    if_123_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_123_5_end:
    mov qword [rbx + 45], 0
    loop_129_5:
        mov r15, qword [rbx + 45]
        mov r14, 130
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 130
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
        if_133_12:
        cmp_133_12:
        cmp qword [rbx + 37], 20
        jne if_133_9_end
        if_133_12_code:
            jmp loop_129_5_end
        if_133_9_end:
    jmp loop_129_5
    loop_129_5_end:
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
;[33:1] # exit is a built-in function
;[40:1] # function arguments and return are equivalent to mutable references
;[47:1] # default argument type is i64 on x86_64 and i32 on rv32i
;[48:1] # arguments are references to memory locations
;[55:1] # return is a reference to the target with optional type
;[56:1] # it is accessed as a variable, in this case `res`
;[66:1] # array arguments are declared with [] and optional type
;[91:7] const yes = 1
;[92:7] const no = 0
;[93:7] const maybe = -1
;[95:1] # constants can be declared in any scope and shadow outer declarations
;[98:1] # limited support for non-inlined functions
;[99:1] # arguments and return are references to memory locations
;[100:1] # arrays not supported
; 
main:
;   [140:5] var arr[4] i32
;   [140:9] arr: i32[4] (16 B @ [rbp + 224])
;   [140:9] zero 4 * 4 B = 16 B
;   [140:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [141:5] # arrays are initialized to 0
;   [143:5] var answer
;   [143:9] answer: i64 (8 B @ [rbp + 240])
;   [143:9] zero 1 * 8 B = 8 B
;   [143:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [144:5] assert(answer == 0)
;   [144:12] allocate scratch register -> r15
;   [144:12] ? answer == 0
;   [144:12] ? answer == 0
    cmp_144_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_144_12:
;   [32:6] assert(x bool)
    assert_144_5:
;       [144:5] alias x -> r15b
        if_32_26_144_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_144_5:
        cmp r15b, 0
        jne if_32_23_144_5_end
        if_32_26_144_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_144_5_end:
;       [144:5] free scratch register r15
    assert_144_5_end:
;   [145:5] # variables without initializer are zeroed
;   [147:5] answer = maybe
;   [147:14] maybe
    mov qword [rbp + 240], -1
;   [148:5] assert(answer == -1)
;   [148:12] allocate scratch register -> r15
;   [148:12] ? answer == -1
;   [148:12] ? answer == -1
    cmp_148_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_148_12:
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
;       [151:9] # a code block opens a new scope
;       [152:9] # constants and variables shadow outer scope
;       [153:15] const maybe = 33
;       [154:9] assert(maybe == 33)
;       [154:16] allocate scratch register -> r15
;       [154:16] ? maybe == 33
;       [154:16] ? maybe == 33
        cmp_154_16:
;       [154:16] const eval to true
        bool_end_154_16:
        mov r15b, 1
;       [32:6] assert(x bool)
        assert_154_9:
;           [154:9] alias x -> r15b
            if_32_26_154_9:
;           [32:26] ? not x
;           [32:26] ? not x
            cmp_32_26_154_9:
            cmp r15b, 0
            jne if_32_23_154_9_end
            if_32_26_154_9_code:
;               [32:32] exit(1)
;               [32:32] allocate named register rdi
;               [32:37] 1
                mov rdi, 1
                mov rax, 60
                syscall
;               [32:32] free named register rdi
            if_32_23_154_9_end:
;           [154:9] free scratch register r15
        assert_154_9_end:
;   [157:5] assert(maybe == -1)
;   [157:12] allocate scratch register -> r15
;   [157:12] ? maybe == -1
;   [157:12] ? maybe == -1
    cmp_157_12:
;   [157:12] const eval to true
    bool_end_157_12:
    mov r15b, 1
;   [32:6] assert(x bool)
    assert_157_5:
;       [157:5] alias x -> r15b
        if_32_26_157_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_157_5:
        cmp r15b, 0
        jne if_32_23_157_5_end
        if_32_26_157_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_157_5_end:
;       [157:5] free scratch register r15
    assert_157_5_end:
;   [159:5] var ix = 1
;   [159:9] ix: i64 (8 B @ [rbp + 248])
;   [159:9] ix = 1
;   [159:14] 1
    mov qword [rbp + 248], 1
;   [160:5] # variables can have an initial value that can be an expression
;   [162:5] arr[ix] = 2
;   [162:5] allocate scratch register -> r15
;   [162:9] set array index
;   [162:9] ix
    mov r15, qword [rbp + 248]
;   [162:9] bounds check
;   [162:9] allocate scratch register -> r14
;   [162:9] line number
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [162:9] free scratch register r14
;   [162:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [162:5] free scratch register r15
;   [163:5] arr[ix + 1] = arr[ix]
;   [163:5] allocate scratch register -> r15
;   [163:9] set array index
;   [163:9] ix
    mov r15, qword [rbp + 248]
;   [163:14] r15 + 1
    add r15, 1
;   [163:9] bounds check
;   [163:9] allocate scratch register -> r14
;   [163:9] line number
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [163:9] free scratch register r14
;   [163:19] arr[ix]
;   [163:19] allocate scratch register -> r14
;   [163:23] set array index
;   [163:23] ix
    mov r14, qword [rbp + 248]
;   [163:23] bounds check
;   [163:23] allocate scratch register -> r13
;   [163:23] line number
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [163:23] free scratch register r13
;   [163:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [163:19] free scratch register r13
;   [163:19] free scratch register r14
;   [163:5] free scratch register r15
;   [164:5] assert(arr[1] == 2)
;   [164:12] allocate scratch register -> r15
;   [164:12] ? arr[1] == 2
;   [164:12] ? arr[1] == 2
    cmp_164_12:
;   [164:12] allocate scratch register -> r14
;   [164:16] set array index
;   [164:16] 1
    mov r14, 1
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
;   [165:5] assert(arr[2] == 2)
;   [165:12] allocate scratch register -> r15
;   [165:12] ? arr[2] == 2
;   [165:12] ? arr[2] == 2
    cmp_165_12:
;   [165:12] allocate scratch register -> r14
;   [165:16] set array index
;   [165:16] 2
    mov r14, 2
;   [165:16] bounds check
;   [165:16] allocate scratch register -> r13
;   [165:16] line number
    mov r13, 165
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [165:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [165:12] free scratch register r14
    sete r15b
    bool_end_165_12:
;   [32:6] assert(x bool)
    assert_165_5:
;       [165:5] alias x -> r15b
        if_32_26_165_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_165_5:
        cmp r15b, 0
        jne if_32_23_165_5_end
        if_32_26_165_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_165_5_end:
;       [165:5] free scratch register r15
    assert_165_5_end:
;   [167:5] array_copy(arr[2], arr, 2)
;   [167:5] allocate named register rsi
;   [167:5] allocate named register rdi
;   [167:5] allocate named register rcx
;   [167:29] 2
;   [167:29] 2
    mov rcx, 2
;   [167:16] arr[2]
;   [167:16] allocate scratch register -> r15
;   [167:20] set array index
;   [167:20] 2
    mov r15, 2
;   [167:20] bounds check
;   [167:20] allocate scratch register -> r14
;   [167:20] line number
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [167:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [167:20] free scratch register r13
    cmovg rbp, r14
    jg baz_bounds_panic
;   [167:20] free scratch register r14
    lea rsi, [rbp + r15 * 4 + 224]
;   [167:5] free scratch register r15
;   [167:24] arr
;   [167:24] bounds check
;   [167:24] allocate scratch register -> r15
;   [167:24] line number
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [167:24] free scratch register r15
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
;   [167:5] free named register rcx
;   [167:5] free named register rdi
;   [167:5] free named register rsi
;   [168:5] assert(arr[0] == 2)
;   [168:12] allocate scratch register -> r15
;   [168:12] ? arr[0] == 2
;   [168:12] ? arr[0] == 2
    cmp_168_12:
;   [168:12] allocate scratch register -> r14
;   [168:16] set array index
;   [168:16] 0
    mov r14, 0
;   [168:16] bounds check
;   [168:16] allocate scratch register -> r13
;   [168:16] line number
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [168:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [168:12] free scratch register r14
    sete r15b
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
;   [169:5] # `array_copy` is a built-in function: copy from, to, number of elements
;   [171:5] var arr1[8] i32
;   [171:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [171:9] zero 8 * 4 B = 32 B
;   [171:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [172:5] array_copy(arr, arr1, 4)
;   [172:5] allocate named register rsi
;   [172:5] allocate named register rdi
;   [172:5] allocate named register rcx
;   [172:27] 4
;   [172:27] 4
    mov rcx, 4
;   [172:16] arr
;   [172:16] bounds check
;   [172:16] allocate scratch register -> r15
;   [172:16] line number
    mov r15, 172
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [172:16] free scratch register r15
    lea rsi, [rbp + 224]
;   [172:21] arr1
;   [172:21] bounds check
;   [172:21] allocate scratch register -> r15
;   [172:21] line number
    mov r15, 172
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [172:21] free scratch register r15
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
;   [172:5] free named register rcx
;   [172:5] free named register rdi
;   [172:5] free named register rsi
;   [173:5] var eq bool = arrays_equal(arr[1], arr1[1], 3)
;   [173:9] eq: bool (1 B @ [rbp + 288])
;   [173:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [173:19] ? arrays_equal(arr[1], arr1[1], 3)
;   [173:19] ? arrays_equal(arr[1], arr1[1], 3)
    cmp_173_19:
;       [173:19] arrays_equal(arr[1], arr1[1], 3)
;       [173:19] allocate named register rsi
;       [173:19] allocate named register rdi
;       [173:19] allocate named register rcx
;       [173:49] 3
;       [173:49] 3
        mov rcx, 3
;       [173:32] arr[1]
;       [173:32] allocate scratch register -> r15
;       [173:36] set array index
;       [173:36] 1
        mov r15, 1
;       [173:36] bounds check
;       [173:36] allocate scratch register -> r14
;       [173:36] line number
        mov r14, 173
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
;       [173:36] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 4
;       [173:36] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [173:36] free scratch register r14
        lea rsi, [rbp + r15 * 4 + 224]
;       [173:19] free scratch register r15
;       [173:40] arr1[1]
;       [173:40] allocate scratch register -> r15
;       [173:45] set array index
;       [173:45] 1
        mov r15, 1
;       [173:45] bounds check
;       [173:45] allocate scratch register -> r14
;       [173:45] line number
        mov r14, 173
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
;       [173:45] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 8
;       [173:45] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [173:45] free scratch register r14
        lea rdi, [rbp + r15 * 4 + 256]
;       [173:19] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [173:19] free named register rcx
;       [173:19] free named register rdi
;       [173:19] free named register rsi
        sete byte [rbp + 288]
    bool_end_173_19:
;   [174:5] # type `bool` is built-in
;   [175:5] # `arrays_equal` is built-in function comparing source and destination
;   [176:5] assert(eq)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? eq
;   [176:12] ? eq
    cmp_176_12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool_end_176_12:
;   [32:6] assert(x bool)
    assert_176_5:
;       [176:5] alias x -> r15b
        if_32_26_176_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_176_5:
        cmp r15b, 0
        jne if_32_23_176_5_end
        if_32_26_176_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_176_5_end:
;       [176:5] free scratch register r15
    assert_176_5_end:
;   [178:5] arr1[2] = -1
;   [178:5] allocate scratch register -> r15
;   [178:10] set array index
;   [178:10] 2
    mov r15, 2
;   [178:10] bounds check
;   [178:10] allocate scratch register -> r14
;   [178:10] line number
    mov r14, 178
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [178:10] free scratch register r14
;   [178:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [178:5] free scratch register r15
;   [179:5] assert(not arrays_equal(arr, arr1, 4))
;   [179:12] allocate scratch register -> r15
;   [179:12] ? not arrays_equal(arr, arr1, 4)
;   [179:12] ? not arrays_equal(arr, arr1, 4)
    cmp_179_12:
;       [179:16] arrays_equal(arr, arr1, 4)
;       [179:16] allocate named register rsi
;       [179:16] allocate named register rdi
;       [179:16] allocate named register rcx
;       [179:40] 4
;       [179:40] 4
        mov rcx, 4
;       [179:29] arr
;       [179:29] bounds check
;       [179:29] allocate scratch register -> r14
;       [179:29] line number
        mov r14, 179
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
;       [179:29] free scratch register r14
        lea rsi, [rbp + 224]
;       [179:34] arr1
;       [179:34] bounds check
;       [179:34] allocate scratch register -> r14
;       [179:34] line number
        mov r14, 179
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [179:34] free scratch register r14
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [179:16] free named register rcx
;       [179:16] free named register rdi
;       [179:16] free named register rsi
        setne r15b
    bool_end_179_12:
;   [32:6] assert(x bool)
    assert_179_5:
;       [179:5] alias x -> r15b
        if_32_26_179_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_179_5:
        cmp r15b, 0
        jne if_32_23_179_5_end
        if_32_26_179_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_179_5_end:
;       [179:5] free scratch register r15
    assert_179_5_end:
;   [181:5] ix = 3
;   [181:10] 3
    mov qword [rbp + 248], 3
;   [182:5] arr[ix] = ~inv(arr[ix - 1])
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
;   [182:16] arr = ~inv(arr[ix - 1])
;   [182:16] = expression
;   [182:16] ~inv(arr[ix - 1])
;   [182:20] allocate scratch register -> r14
;   [182:24] set array index
;   [182:24] ix
    mov r14, qword [rbp + 248]
;   [182:29] r14 - 1
    sub r14, 1
;   [182:24] bounds check
;   [182:24] allocate scratch register -> r13
;   [182:24] line number
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [182:24] free scratch register r13
;   [58:6] inv(i i32) res i32
    inv_182_16:
;       [182:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [182:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [59:5] res = ~i
;       [59:12] ~i
;       [59:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [59:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [182:16] free scratch register r14
    inv_182_16_end:
    not dword [rbp + r15 * 4 + 224]
;   [182:5] free scratch register r15
;   [183:5] assert(arr[ix] == 2)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? arr[ix] == 2
;   [183:12] ? arr[ix] == 2
    cmp_183_12:
;   [183:12] allocate scratch register -> r14
;   [183:16] set array index
;   [183:16] ix
    mov r14, qword [rbp + 248]
;   [183:16] bounds check
;   [183:16] allocate scratch register -> r13
;   [183:16] line number
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [183:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [183:12] free scratch register r14
    sete r15b
    bool_end_183_12:
;   [32:6] assert(x bool)
    assert_183_5:
;       [183:5] alias x -> r15b
        if_32_26_183_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_183_5:
        cmp r15b, 0
        jne if_32_23_183_5_end
        if_32_26_183_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_183_5_end:
;       [183:5] free scratch register r15
    assert_183_5_end:
;   [185:5] faz(arr)
;   [68:6] faz(arg[] i32)
    faz_185_5:
;       [185:5] alias arg -> arr
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
    faz_185_5_end:
;   [186:5] assert(arr[1] == 0xfe)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? arr[1] == 0xfe
;   [186:12] ? arr[1] == 0xfe
    cmp_186_12:
;   [186:12] allocate scratch register -> r14
;   [186:16] set array index
;   [186:16] 1
    mov r14, 1
;   [186:16] bounds check
;   [186:16] allocate scratch register -> r13
;   [186:16] line number
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [186:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 254
;   [186:12] free scratch register r14
    sete r15b
    bool_end_186_12:
;   [32:6] assert(x bool)
    assert_186_5:
;       [186:5] alias x -> r15b
        if_32_26_186_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_186_5:
        cmp r15b, 0
        jne if_32_23_186_5_end
        if_32_26_186_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_186_5_end:
;       [186:5] free scratch register r15
    assert_186_5_end:
;   [188:5] var arr3[] = { 3, 5 }
;   [188:9] arr3: i64[2] (16 B @ [rbp + 289])
;   [188:9] arr3= { 3, 5 }
;   [188:20] [0]
;   [188:20] 3
    mov qword [rbp + 289], 3
;   [188:20] [1]
;   [188:23] 5
    mov qword [rbp + 297], 5
;   [189:5] foo arr3
;   [189:9] allocate scratch register -> r15
;   [189:9] e: i64 (r15)
;   [189:9] i: i64 (8 B @ [rbp + 313])
;   [189:9] const n = 2
;   [189:9] initiate iterator e
    lea r15, [rbp + 289]
;   [189:9] initiate counter i
    mov qword [rbp + 313], 0
    foo_189_5:
;       [190:9] e = e + i + n
;       [190:13] instructions without scratch register 3, with 4
;       [190:13] e
;       [190:17] e + i
;       [190:17] allocate scratch register -> r14
        mov r14, qword [rbp + 313]
        add qword [r15], r14
;       [190:17] free scratch register r14
;       [190:21] e + n
        add qword [r15], 2
        foo_189_5_continue:
            add r15, 8
            inc qword [rbp + 313]
            cmp qword [rbp + 313], 2
            jne foo_189_5
    foo_189_5_end:
;   [189:5] free scratch register r15
;   [192:5] assert(arr3[0] == 3 + 0 + 2)
;   [192:12] allocate scratch register -> r15
;   [192:12] ? arr3[0] == 3 + 0 + 2
;   [192:12] ? arr3[0] == 3 + 0 + 2
    cmp_192_12:
;   [192:12] allocate scratch register -> r14
;   [192:17] set array index
;   [192:17] 0
    mov r14, 0
;   [192:17] bounds check
;   [192:17] allocate scratch register -> r13
;   [192:17] line number
    mov r13, 192
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [192:17] free scratch register r13
;   [192:23] allocate scratch register -> r13
;       [192:23] 3
        mov r13, 3
;       [192:27] r13 + 0
        add r13, 0
;       [192:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
;   [192:12] free scratch register r13
;   [192:12] free scratch register r14
    sete r15b
    bool_end_192_12:
;   [32:6] assert(x bool)
    assert_192_5:
;       [192:5] alias x -> r15b
        if_32_26_192_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_192_5:
        cmp r15b, 0
        jne if_32_23_192_5_end
        if_32_26_192_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_192_5_end:
;       [192:5] free scratch register r15
    assert_192_5_end:
;   [193:5] assert(arr3[1] == 5 + 1 + 2)
;   [193:12] allocate scratch register -> r15
;   [193:12] ? arr3[1] == 5 + 1 + 2
;   [193:12] ? arr3[1] == 5 + 1 + 2
    cmp_193_12:
;   [193:12] allocate scratch register -> r14
;   [193:17] set array index
;   [193:17] 1
    mov r14, 1
;   [193:17] bounds check
;   [193:17] allocate scratch register -> r13
;   [193:17] line number
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [193:17] free scratch register r13
;   [193:23] allocate scratch register -> r13
;       [193:23] 5
        mov r13, 5
;       [193:27] r13 + 1
        add r13, 1
;       [193:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
;   [193:12] free scratch register r13
;   [193:12] free scratch register r14
    sete r15b
    bool_end_193_12:
;   [32:6] assert(x bool)
    assert_193_5:
;       [193:5] alias x -> r15b
        if_32_26_193_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_193_5:
        cmp r15b, 0
        jne if_32_23_193_5_end
        if_32_26_193_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_193_5_end:
;       [193:5] free scratch register r15
    assert_193_5_end:
;   [194:5] # `foo` is a language construct that iterates over an array injecting:
;   [195:5] # `e`: current element
;   [196:5] # `i`: index starting at 0
;   [197:5] # `n`: constant array size
;   [199:5] var p point = {0, 0}
;   [199:9] p: point (16 B @ [rbp + 305])
;   [199:9] p = {0, 0}
;   [199:20] copy field 'x'
    mov qword [rbp + 305], 0
;   [199:23] copy field 'y'
    mov qword [rbp + 313], 0
;   [200:5] fooz(p)
;   [42:6] fooz(pt point)
    fooz_200_5:
;       [200:5] alias pt -> p
;       [43:5] pt.x = 0b10
;       [43:12] 0b10
        mov qword [rbp + 305], 2
;       [43:20] # binary value 2
;       [44:5] pt.y = 0xb
;       [44:12] 0xb
        mov qword [rbp + 313], 11
;       [44:20] # hex value 11
    fooz_200_5_end:
;   [201:5] assert(p.x == 2)
;   [201:12] allocate scratch register -> r15
;   [201:12] ? p.x == 2
;   [201:12] ? p.x == 2
    cmp_201_12:
    cmp qword [rbp + 305], 2
    sete r15b
    bool_end_201_12:
;   [32:6] assert(x bool)
    assert_201_5:
;       [201:5] alias x -> r15b
        if_32_26_201_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_201_5:
        cmp r15b, 0
        jne if_32_23_201_5_end
        if_32_26_201_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_201_5_end:
;       [201:5] free scratch register r15
    assert_201_5_end:
;   [202:5] assert(p.y == 0xb)
;   [202:12] allocate scratch register -> r15
;   [202:12] ? p.y == 0xb
;   [202:12] ? p.y == 0xb
    cmp_202_12:
    cmp qword [rbp + 313], 11
    sete r15b
    bool_end_202_12:
;   [32:6] assert(x bool)
    assert_202_5:
;       [202:5] alias x -> r15b
        if_32_26_202_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_202_5:
        cmp r15b, 0
        jne if_32_23_202_5_end
        if_32_26_202_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_202_5_end:
;       [202:5] free scratch register r15
    assert_202_5_end:
;   [204:5] var q point = p
;   [204:9] q: point (16 B @ [rbp + 321])
;   [204:9] q = p
;   [204:19] size <= 16 B, use mov
;   [204:19] allocate named register rax
    mov rax, qword [rbp + 305]
    mov qword [rbp + 321], rax
    mov rax, qword [rbp + 313]
    mov qword [rbp + 329], rax
;   [204:19] free named register rax
;   [205:5] assert(equal(p, q))
;   [205:12] allocate scratch register -> r15
;   [205:12] ? equal(p, q)
;   [205:12] ? equal(p, q)
    cmp_205_12:
;       [205:12] equal(p, q)
;       [205:12] allocate named register rsi
;       [205:12] allocate named register rdi
;       [205:12] allocate named register rcx
;       [205:18] p
        lea rsi, [rbp + 305]
;       [205:21] q
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
;       [205:12] free named register rcx
;       [205:12] free named register rdi
;       [205:12] free named register rsi
        sete r15b
    bool_end_205_12:
;   [32:6] assert(x bool)
    assert_205_5:
;       [205:5] alias x -> r15b
        if_32_26_205_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_205_5:
        cmp r15b, 0
        jne if_32_23_205_5_end
        if_32_26_205_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_205_5_end:
;       [205:5] free scratch register r15
    assert_205_5_end:
;   [206:5] # `equal` is built-in function to compare user types for equality or same
;   [207:5] # size arrays
;   [209:5] q.x = 3
;   [209:11] 3
    mov qword [rbp + 321], 3
;   [210:5] assert(not equal(p, q))
;   [210:12] allocate scratch register -> r15
;   [210:12] ? not equal(p, q)
;   [210:12] ? not equal(p, q)
    cmp_210_12:
;       [210:16] equal(p, q)
;       [210:16] allocate named register rsi
;       [210:16] allocate named register rdi
;       [210:16] allocate named register rcx
;       [210:22] p
        lea rsi, [rbp + 305]
;       [210:25] q
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
;       [210:16] free named register rcx
;       [210:16] free named register rdi
;       [210:16] free named register rsi
        setne r15b
    bool_end_210_12:
;   [32:6] assert(x bool)
    assert_210_5:
;       [210:5] alias x -> r15b
        if_32_26_210_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_210_5:
        cmp r15b, 0
        jne if_32_23_210_5_end
        if_32_26_210_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_210_5_end:
;       [210:5] free scratch register r15
    assert_210_5_end:
;   [212:5] var i = 0
;   [212:9] i: i64 (8 B @ [rbp + 337])
;   [212:9] i = 0
;   [212:13] 0
    mov qword [rbp + 337], 0
;   [213:5] bar(i)
;   [50:6] bar(arg)
    bar_213_5:
;       [213:5] alias arg -> i
        if_51_8_213_5:
;       [51:8] ? arg == 0
;       [51:8] ? arg == 0
        cmp_51_8_213_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_213_5_end
        if_51_8_213_5_code:
;           [51:17] return
            jmp bar_213_5_end
        if_51_5_213_5_end:
;       [52:5] arg = 0xff
;       [52:11] 0xff
        mov qword [rbp + 337], 255
    bar_213_5_end:
;   [214:5] assert(i == 0)
;   [214:12] allocate scratch register -> r15
;   [214:12] ? i == 0
;   [214:12] ? i == 0
    cmp_214_12:
    cmp qword [rbp + 337], 0
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
;   [216:5] i = 1
;   [216:9] 1
    mov qword [rbp + 337], 1
;   [217:5] bar(i)
;   [50:6] bar(arg)
    bar_217_5:
;       [217:5] alias arg -> i
        if_51_8_217_5:
;       [51:8] ? arg == 0
;       [51:8] ? arg == 0
        cmp_51_8_217_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_217_5_end
        if_51_8_217_5_code:
;           [51:17] return
            jmp bar_217_5_end
        if_51_5_217_5_end:
;       [52:5] arg = 0xff
;       [52:11] 0xff
        mov qword [rbp + 337], 255
    bar_217_5_end:
;   [218:5] assert(i == 0xff)
;   [218:12] allocate scratch register -> r15
;   [218:12] ? i == 0xff
;   [218:12] ? i == 0xff
    cmp_218_12:
    cmp qword [rbp + 337], 255
    sete r15b
    bool_end_218_12:
;   [32:6] assert(x bool)
    assert_218_5:
;       [218:5] alias x -> r15b
        if_32_26_218_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_218_5:
        cmp r15b, 0
        jne if_32_23_218_5_end
        if_32_26_218_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_218_5_end:
;       [218:5] free scratch register r15
    assert_218_5_end:
;   [220:5] var j = 1
;   [220:9] j: i64 (8 B @ [rbp + 345])
;   [220:9] j = 1
;   [220:13] 1
    mov qword [rbp + 345], 1
;   [221:5] var k = baz(j)
;   [221:9] k: i64 (8 B @ [rbp + 353])
;   [221:9] k = baz(j)
;   [221:13] k = baz(j)
;   [221:13] = expression
;   [221:13] baz(j)
;   [62:6] baz(arg) res
    baz_221_13:
;       [221:13] alias res -> k
;       [221:13] alias arg -> j
;       [63:5] res = arg * 2
;       [63:11] instructions without scratch register 5, with 3
;       [63:11] allocate scratch register -> r15
;       [63:11] arg
        mov r15, qword [rbp + 345]
;       [63:17] r15 * 2
;       [63:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 353], r15
;       [63:11] free scratch register r15
    baz_221_13_end:
;   [222:5] assert(k == 2)
;   [222:12] allocate scratch register -> r15
;   [222:12] ? k == 2
;   [222:12] ? k == 2
    cmp_222_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_222_12:
;   [32:6] assert(x bool)
    assert_222_5:
;       [222:5] alias x -> r15b
        if_32_26_222_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_222_5:
        cmp r15b, 0
        jne if_32_23_222_5_end
        if_32_26_222_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_222_5_end:
;       [222:5] free scratch register r15
    assert_222_5_end:
;   [224:5] k = baz(1)
;   [224:9] k = baz(1)
;   [224:9] = expression
;   [224:9] baz(1)
;   [62:6] baz(arg) res
    baz_224_9:
;       [224:9] alias res -> k
;       [224:9] alias arg -> 1
;       [63:5] res = arg * 2
;       [63:11] instructions without scratch register 4, with 3
;       [63:11] allocate scratch register -> r15
;       [63:11] arg
        mov r15, 1
;       [63:17] r15 * 2
;       [63:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 353], r15
;       [63:11] free scratch register r15
    baz_224_9_end:
;   [225:5] assert(k == 2)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? k == 2
;   [225:12] ? k == 2
    cmp_225_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_225_12:
;   [32:6] assert(x bool)
    assert_225_5:
;       [225:5] alias x -> r15b
        if_32_26_225_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_225_5:
        cmp r15b, 0
        jne if_32_23_225_5_end
        if_32_26_225_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_225_5_end:
;       [225:5] free scratch register r15
    assert_225_5_end:
;   [227:5] var p0 point = {baz(3), 0}
;   [227:9] p0: point (16 B @ [rbp + 361])
;   [227:9] p0 = {baz(3), 0}
;   [227:21] copy field 'x'
;   [227:21] p0.x = baz(3)
;   [227:21] = expression
;   [227:21] baz(3)
;   [62:6] baz(arg) res
    baz_227_21:
;       [227:21] alias res -> p0.x (lea: rbp + 361)
;       [227:21] alias arg -> 3
;       [63:5] res = arg * 2
;       [63:11] instructions without scratch register 4, with 3
;       [63:11] allocate scratch register -> r15
;       [63:11] arg
        mov r15, 3
;       [63:17] r15 * 2
;       [63:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 361], r15
;       [63:11] free scratch register r15
    baz_227_21_end:
;   [227:29] copy field 'y'
    mov qword [rbp + 369], 0
;   [228:5] assert(p0.x == 6)
;   [228:12] allocate scratch register -> r15
;   [228:12] ? p0.x == 6
;   [228:12] ? p0.x == 6
    cmp_228_12:
    cmp qword [rbp + 361], 6
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
;   [230:5] var pt point = point_init()
;   [230:9] pt: point (16 B @ [rbp + 377])
;   [230:9] pt = point_init()
;   [230:20] point_init()
;   [82:6] point_init() res point
    point_init_230_20:
;       [230:20] alias res -> pt
;       [83:5] res.x = -1
;       [83:14] -1
        mov qword [rbp + 377], -1
;       [84:5] res.y = -2
;       [84:14] -2
        mov qword [rbp + 385], -2
    point_init_230_20_end:
;   [231:5] assert(pt.x == -1)
;   [231:12] allocate scratch register -> r15
;   [231:12] ? pt.x == -1
;   [231:12] ? pt.x == -1
    cmp_231_12:
    cmp qword [rbp + 377], -1
    sete r15b
    bool_end_231_12:
;   [32:6] assert(x bool)
    assert_231_5:
;       [231:5] alias x -> r15b
        if_32_26_231_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_231_5:
        cmp r15b, 0
        jne if_32_23_231_5_end
        if_32_26_231_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_231_5_end:
;       [231:5] free scratch register r15
    assert_231_5_end:
;   [232:5] assert(pt.y == -2)
;   [232:12] allocate scratch register -> r15
;   [232:12] ? pt.y == -2
;   [232:12] ? pt.y == -2
    cmp_232_12:
    cmp qword [rbp + 385], -2
    sete r15b
    bool_end_232_12:
;   [32:6] assert(x bool)
    assert_232_5:
;       [232:5] alias x -> r15b
        if_32_26_232_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_232_5:
        cmp r15b, 0
        jne if_32_23_232_5_end
        if_32_26_232_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_232_5_end:
;       [232:5] free scratch register r15
    assert_232_5_end:
;   [234:5] var x = 1
;   [234:9] x: i64 (8 B @ [rbp + 393])
;   [234:9] x = 1
;   [234:13] 1
    mov qword [rbp + 393], 1
;   [235:5] var y = 2
;   [235:9] y: i64 (8 B @ [rbp + 401])
;   [235:9] y = 2
;   [235:13] 2
    mov qword [rbp + 401], 2
;   [237:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [237:9] o1: object (20 B @ [rbp + 409])
;   [237:9] o1 = {{x * 10, y}, 0xff0000}
;   [237:22] copy field 'pos'
;   [237:23] copy field 'x'
;   [237:23] instructions without scratch register 5, with 3
;   [237:23] allocate scratch register -> r15
;   [237:23] x
    mov r15, qword [rbp + 393]
;   [237:27] r15 * 10
;   [237:27] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 409], r15
;   [237:23] free scratch register r15
;   [237:31] copy field 'y'
;   [237:31] allocate scratch register -> r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 417], r15
;   [237:31] free scratch register r15
;   [237:35] copy field 'color'
    mov dword [rbp + 425], 16711680
;   [238:5] assert(o1.pos.x == 10)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? o1.pos.x == 10
;   [238:12] ? o1.pos.x == 10
    cmp_238_12:
    cmp qword [rbp + 409], 10
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
;   [239:5] assert(o1.pos.y == 2)
;   [239:12] allocate scratch register -> r15
;   [239:12] ? o1.pos.y == 2
;   [239:12] ? o1.pos.y == 2
    cmp_239_12:
    cmp qword [rbp + 417], 2
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
;   [240:5] assert(o1.color == 0xff0000)
;   [240:12] allocate scratch register -> r15
;   [240:12] ? o1.color == 0xff0000
;   [240:12] ? o1.color == 0xff0000
    cmp_240_12:
    cmp dword [rbp + 425], 16711680
    sete r15b
    bool_end_240_12:
;   [32:6] assert(x bool)
    assert_240_5:
;       [240:5] alias x -> r15b
        if_32_26_240_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_240_5:
        cmp r15b, 0
        jne if_32_23_240_5_end
        if_32_26_240_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_240_5_end:
;       [240:5] free scratch register r15
    assert_240_5_end:
;   [242:5] var p1 point = {-x, -y}
;   [242:9] p1: point (16 B @ [rbp + 429])
;   [242:9] p1 = {-x, -y}
;   [242:21] copy field 'x'
;   [242:21] allocate scratch register -> r15
    mov r15, qword [rbp + 393]
    mov qword [rbp + 429], r15
;   [242:21] free scratch register r15
    neg qword [rbp + 429]
;   [242:25] copy field 'y'
;   [242:25] allocate scratch register -> r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 437], r15
;   [242:25] free scratch register r15
    neg qword [rbp + 437]
;   [243:5] o1.pos = p1
;   [243:14] size <= 16 B, use mov
;   [243:14] allocate named register rax
    mov rax, qword [rbp + 429]
    mov qword [rbp + 409], rax
    mov rax, qword [rbp + 437]
    mov qword [rbp + 417], rax
;   [243:14] free named register rax
;   [244:5] assert(o1.pos.x == -1)
;   [244:12] allocate scratch register -> r15
;   [244:12] ? o1.pos.x == -1
;   [244:12] ? o1.pos.x == -1
    cmp_244_12:
    cmp qword [rbp + 409], -1
    sete r15b
    bool_end_244_12:
;   [32:6] assert(x bool)
    assert_244_5:
;       [244:5] alias x -> r15b
        if_32_26_244_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_244_5:
        cmp r15b, 0
        jne if_32_23_244_5_end
        if_32_26_244_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_244_5_end:
;       [244:5] free scratch register r15
    assert_244_5_end:
;   [245:5] assert(o1.pos.y == -2)
;   [245:12] allocate scratch register -> r15
;   [245:12] ? o1.pos.y == -2
;   [245:12] ? o1.pos.y == -2
    cmp_245_12:
    cmp qword [rbp + 417], -2
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
;   [247:5] var o2 object = o1
;   [247:9] o2: object (20 B @ [rbp + 445])
;   [247:9] o2 = o1
;   [247:21] allocate named register rsi
;   [247:21] allocate named register rdi
;   [247:21] allocate named register rcx
    lea rsi, [rbp + 409]
    lea rdi, [rbp + 445]
    mov rcx, 20
    rep movsb
;   [247:21] free named register rcx
;   [247:21] free named register rdi
;   [247:21] free named register rsi
;   [248:5] assert(o2.pos.x == -1)
;   [248:12] allocate scratch register -> r15
;   [248:12] ? o2.pos.x == -1
;   [248:12] ? o2.pos.x == -1
    cmp_248_12:
    cmp qword [rbp + 445], -1
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
;   [249:5] assert(o2.pos.y == -2)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? o2.pos.y == -2
;   [249:12] ? o2.pos.y == -2
    cmp_249_12:
    cmp qword [rbp + 453], -2
    sete r15b
    bool_end_249_12:
;   [32:6] assert(x bool)
    assert_249_5:
;       [249:5] alias x -> r15b
        if_32_26_249_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_249_5:
        cmp r15b, 0
        jne if_32_23_249_5_end
        if_32_26_249_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_249_5_end:
;       [249:5] free scratch register r15
    assert_249_5_end:
;   [250:5] assert(o2.color == 0xff0000)
;   [250:12] allocate scratch register -> r15
;   [250:12] ? o2.color == 0xff0000
;   [250:12] ? o2.color == 0xff0000
    cmp_250_12:
    cmp dword [rbp + 461], 16711680
    sete r15b
    bool_end_250_12:
;   [32:6] assert(x bool)
    assert_250_5:
;       [250:5] alias x -> r15b
        if_32_26_250_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_250_5:
        cmp r15b, 0
        jne if_32_23_250_5_end
        if_32_26_250_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_250_5_end:
;       [250:5] free scratch register r15
    assert_250_5_end:
;   [252:5] var o3[2] object
;   [252:9] o3: object[2] (40 B @ [rbp + 465])
;   [252:9] zero 2 * 20 B = 40 B
;   [252:5] allocate named register rax
;   [252:5] allocate named register rdi
;   [252:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 465]
    mov rcx, 40
    rep stosb
;   [252:5] free named register rcx
;   [252:5] free named register rdi
;   [252:5] free named register rax
;   [253:5] o3.pos.y = 73
;   [253:16] 73
    mov qword [rbp + 473], 73
;   [254:5] # index 0 in an array can be accessed without array index
;   [256:5] assert(o3[0].pos.y == 73)
;   [256:12] allocate scratch register -> r15
;   [256:12] ? o3[0].pos.y == 73
;   [256:12] ? o3[0].pos.y == 73
    cmp_256_12:
;   [256:12] allocate scratch register -> r14
    lea r14, [rbp + 465]
;   [256:12] allocate scratch register -> r13
;   [256:15] set array index
;   [256:15] 0
    mov r13, 0
;   [256:15] bounds check
;   [256:15] allocate scratch register -> r12
;   [256:15] line number
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [256:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [256:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [256:12] free scratch register r14
    sete r15b
    bool_end_256_12:
;   [32:6] assert(x bool)
    assert_256_5:
;       [256:5] alias x -> r15b
        if_32_26_256_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_256_5:
        cmp r15b, 0
        jne if_32_23_256_5_end
        if_32_26_256_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_256_5_end:
;       [256:5] free scratch register r15
    assert_256_5_end:
;   [258:5] o3[1] = object_init()
;   [258:5] allocate scratch register -> r15
    lea r15, [rbp + 465]
;   [258:5] allocate scratch register -> r14
;   [258:8] set array index
;   [258:8] 1
    mov r14, 1
;   [258:8] bounds check
;   [258:8] allocate scratch register -> r13
;   [258:8] line number
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [258:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [258:5] free scratch register r14
;   [258:13] object_init()
;   [87:6] object_init() res object
    object_init_258_13:
;       [258:13] alias res -> o3 (lea: r15)
;       [88:5] res.pos.y = 74
;       [88:17] 74
        mov qword [r15 + 8], 74
    object_init_258_13_end:
;   [258:5] free scratch register r15
;   [259:5] assert(o3[1].pos.y == 74)
;   [259:12] allocate scratch register -> r15
;   [259:12] ? o3[1].pos.y == 74
;   [259:12] ? o3[1].pos.y == 74
    cmp_259_12:
;   [259:12] allocate scratch register -> r14
    lea r14, [rbp + 465]
;   [259:12] allocate scratch register -> r13
;   [259:15] set array index
;   [259:15] 1
    mov r13, 1
;   [259:15] bounds check
;   [259:15] allocate scratch register -> r12
;   [259:15] line number
    mov r12, 259
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [259:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [259:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [259:12] free scratch register r14
    sete r15b
    bool_end_259_12:
;   [32:6] assert(x bool)
    assert_259_5:
;       [259:5] alias x -> r15b
        if_32_26_259_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_259_5:
        cmp r15b, 0
        jne if_32_23_259_5_end
        if_32_26_259_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_259_5_end:
;       [259:5] free scratch register r15
    assert_259_5_end:
;   [261:5] var worlds[8] world
;   [261:9] worlds: world[8] (512 B @ [rbp + 505])
;   [261:9] zero 8 * 64 B = 512 B
;   [261:5] allocate named register rax
;   [261:5] allocate named register rdi
;   [261:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 505]
    mov rcx, 512
    rep stosb
;   [261:5] free named register rcx
;   [261:5] free named register rdi
;   [261:5] free named register rax
;   [262:5] worlds[1].locations[1] = 0xffee
;   [262:5] allocate scratch register -> r15
    lea r15, [rbp + 505]
;   [262:5] allocate scratch register -> r14
;   [262:12] set array index
;   [262:12] 1
    mov r14, 1
;   [262:12] bounds check
;   [262:12] allocate scratch register -> r13
;   [262:12] line number
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [262:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [262:5] free scratch register r14
;   [262:5] allocate scratch register -> r14
;   [262:25] set array index
;   [262:25] 1
    mov r14, 1
;   [262:25] bounds check
;   [262:25] allocate scratch register -> r13
;   [262:25] line number
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [262:25] free scratch register r13
;   [262:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [262:5] free scratch register r14
;   [262:5] free scratch register r15
;   [263:5] assert(worlds[1].locations[1] == 0xffee)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? worlds[1].locations[1] == 0xffee
;   [263:12] ? worlds[1].locations[1] == 0xffee
    cmp_263_12:
;   [263:12] allocate scratch register -> r14
    lea r14, [rbp + 505]
;   [263:12] allocate scratch register -> r13
;   [263:19] set array index
;   [263:19] 1
    mov r13, 1
;   [263:19] bounds check
;   [263:19] allocate scratch register -> r12
;   [263:19] line number
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
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
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [263:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [263:12] free scratch register r13
;   [263:12] free scratch register r14
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
;   [265:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [265:5] allocate named register rsi
;   [265:5] allocate named register rdi
;   [265:5] allocate named register rcx
;   [268:9] array_size_of(worlds.locations)
;   [268:9] rcx = array_size_of(worlds.locations)
;   [268:9] = expression
;   [268:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [266:9] worlds[1].locations
;   [266:9] allocate scratch register -> r15
    lea r15, [rbp + 505]
;   [266:9] allocate scratch register -> r14
;   [266:16] set array index
;   [266:16] 1
    mov r14, 1
;   [266:16] bounds check
;   [266:16] allocate scratch register -> r13
;   [266:16] line number
    mov r13, 266
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [266:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [266:9] free scratch register r14
;   [266:9] bounds check
;   [266:9] allocate scratch register -> r14
;   [266:9] line number
    mov r14, 266
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [266:9] free scratch register r14
    lea rsi, [r15]
;   [265:5] free scratch register r15
;   [267:9] worlds[0].locations
;   [267:9] allocate scratch register -> r15
    lea r15, [rbp + 505]
;   [267:9] allocate scratch register -> r14
;   [267:16] set array index
;   [267:16] 0
    mov r14, 0
;   [267:16] bounds check
;   [267:16] allocate scratch register -> r13
;   [267:16] line number
    mov r13, 267
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [267:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [267:9] free scratch register r14
;   [267:9] bounds check
;   [267:9] allocate scratch register -> r14
;   [267:9] line number
    mov r14, 267
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [267:9] free scratch register r14
    lea rdi, [r15]
;   [265:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [265:5] free named register rcx
;   [265:5] free named register rdi
;   [265:5] free named register rsi
;   [270:5] # `array_size_of` is built-in
;   [272:5] assert(worlds[0].locations[1] == 0xffee)
;   [272:12] allocate scratch register -> r15
;   [272:12] ? worlds[0].locations[1] == 0xffee
;   [272:12] ? worlds[0].locations[1] == 0xffee
    cmp_272_12:
;   [272:12] allocate scratch register -> r14
    lea r14, [rbp + 505]
;   [272:12] allocate scratch register -> r13
;   [272:19] set array index
;   [272:19] 0
    mov r13, 0
;   [272:19] bounds check
;   [272:19] allocate scratch register -> r12
;   [272:19] line number
    mov r12, 272
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [272:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [272:12] free scratch register r13
;   [272:12] allocate scratch register -> r13
;   [272:32] set array index
;   [272:32] 1
    mov r13, 1
;   [272:32] bounds check
;   [272:32] allocate scratch register -> r12
;   [272:32] line number
    mov r12, 272
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [272:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [272:12] free scratch register r13
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
;   [273:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [273:12] allocate scratch register -> r15
;   [273:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [273:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_273_12:
;       [273:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [273:12] allocate named register rsi
;       [273:12] allocate named register rdi
;       [273:12] allocate named register rcx
;       [276:14] array_size_of(worlds.locations)
;       [276:14] rcx = array_size_of(worlds.locations)
;       [276:14] = expression
;       [276:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [274:14] worlds[0].locations
;       [274:14] allocate scratch register -> r14
        lea r14, [rbp + 505]
;       [274:14] allocate scratch register -> r13
;       [274:21] set array index
;       [274:21] 0
        mov r13, 0
;       [274:21] bounds check
;       [274:21] allocate scratch register -> r12
;       [274:21] line number
        mov r12, 274
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
;       [274:21] free scratch register r12
        shl r13, 6
        add r14, r13
;       [274:14] free scratch register r13
;       [274:14] bounds check
;       [274:14] allocate scratch register -> r13
;       [274:14] line number
        mov r13, 274
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [274:14] free scratch register r13
        lea rsi, [r14]
;       [273:12] free scratch register r14
;       [275:14] worlds[1].locations
;       [275:14] allocate scratch register -> r14
        lea r14, [rbp + 505]
;       [275:14] allocate scratch register -> r13
;       [275:21] set array index
;       [275:21] 1
        mov r13, 1
;       [275:21] bounds check
;       [275:21] allocate scratch register -> r12
;       [275:21] line number
        mov r12, 275
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
;       [275:21] free scratch register r12
        shl r13, 6
        add r14, r13
;       [275:14] free scratch register r13
;       [275:14] bounds check
;       [275:14] allocate scratch register -> r13
;       [275:14] line number
        mov r13, 275
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [275:14] free scratch register r13
        lea rdi, [r14]
;       [273:12] free scratch register r14
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [273:12] free named register rcx
;       [273:12] free named register rdi
;       [273:12] free named register rsi
        sete r15b
    bool_end_273_12:
;   [32:6] assert(x bool)
    assert_273_5:
;       [273:5] alias x -> r15b
        if_32_26_273_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_273_5:
        cmp r15b, 0
        jne if_32_23_273_5_end
        if_32_26_273_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_273_5_end:
;       [273:5] free scratch register r15
    assert_273_5_end:
;   [279:5] var arr2[] = { -1, 2 }
;   [279:9] arr2: i64[2] (16 B @ [rbp + 1017])
;   [279:9] arr2= { -1, 2 }
;   [279:20] [0]
;   [279:21] -1
    mov qword [rbp + 1017], -1
;   [279:20] [1]
;   [279:24] 2
    mov qword [rbp + 1025], 2
;   [280:5] assert(array_size_of(arr2) == 2)
;   [280:12] allocate scratch register -> r15
;   [280:12] ? array_size_of(arr2) == 2
;   [280:12] ? array_size_of(arr2) == 2
    cmp_280_12:
;   [280:12] allocate scratch register -> r14
;       [280:12] r14 = array_size_of(arr2)
;       [280:12] = expression
;       [280:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [280:12] free scratch register r14
    sete r15b
    bool_end_280_12:
;   [32:6] assert(x bool)
    assert_280_5:
;       [280:5] alias x -> r15b
        if_32_26_280_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_280_5:
        cmp r15b, 0
        jne if_32_23_280_5_end
        if_32_26_280_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_280_5_end:
;       [280:5] free scratch register r15
    assert_280_5_end:
;   [281:5] assert(arr2[0] == -1)
;   [281:12] allocate scratch register -> r15
;   [281:12] ? arr2[0] == -1
;   [281:12] ? arr2[0] == -1
    cmp_281_12:
;   [281:12] allocate scratch register -> r14
;   [281:17] set array index
;   [281:17] 0
    mov r14, 0
;   [281:17] bounds check
;   [281:17] allocate scratch register -> r13
;   [281:17] line number
    mov r13, 281
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [281:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1017], -1
;   [281:12] free scratch register r14
    sete r15b
    bool_end_281_12:
;   [32:6] assert(x bool)
    assert_281_5:
;       [281:5] alias x -> r15b
        if_32_26_281_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_281_5:
        cmp r15b, 0
        jne if_32_23_281_5_end
        if_32_26_281_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_281_5_end:
;       [281:5] free scratch register r15
    assert_281_5_end:
;   [282:5] assert(arr2[1] == 2)
;   [282:12] allocate scratch register -> r15
;   [282:12] ? arr2[1] == 2
;   [282:12] ? arr2[1] == 2
    cmp_282_12:
;   [282:12] allocate scratch register -> r14
;   [282:17] set array index
;   [282:17] 1
    mov r14, 1
;   [282:17] bounds check
;   [282:17] allocate scratch register -> r13
;   [282:17] line number
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [282:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1017], 2
;   [282:12] free scratch register r14
    sete r15b
    bool_end_282_12:
;   [32:6] assert(x bool)
    assert_282_5:
;       [282:5] alias x -> r15b
        if_32_26_282_5:
;       [32:26] ? not x
;       [32:26] ? not x
        cmp_32_26_282_5:
        cmp r15b, 0
        jne if_32_23_282_5_end
        if_32_26_282_5_code:
;           [32:32] exit(1)
;           [32:32] allocate named register rdi
;           [32:37] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:32] free named register rdi
        if_32_23_282_5_end:
;       [282:5] free scratch register r15
    assert_282_5_end:
;   [284:5] var counter
;   [284:9] counter: i64 (8 B @ [rbp + 1033])
;   [284:9] zero 1 * 8 B = 8 B
;   [284:5] size <= 32 B, use mov
    mov qword [rbp + 1033], 0
;   [285:5] var nm str
;   [285:9] nm: str (128 B @ [rbp + 1041])
;   [285:9] zero 1 * 128 B = 128 B
;   [285:5] allocate named register rax
;   [285:5] allocate named register rdi
;   [285:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1041]
    mov rcx, 128
    rep stosb
;   [285:5] free named register rcx
;   [285:5] free named register rdi
;   [285:5] free named register rax
;   [286:5] print(hello)
;   [35:6] print(str[] i8)
    print_286_5:
;       [286:5] alias str -> hello
;       [36:5] write(1, address_of(str), array_size_of(str))
;       [36:5] allocate named register rdi
;       [36:11] 1
        mov rdi, 1
;       [36:5] allocate named register rsi
;       [36:14] rsi = address_of(str)
;       [36:14] = expression
;       [36:14] address_of(str)
        lea rsi, [rbp]
;       [36:5] allocate named register rdx
;       [36:31] rdx = array_size_of(str)
;       [36:31] = expression
;       [36:31] array_size_of(str)
        mov rdx, 21
;       [36:5] allocate named register rax
        mov rax, 1
        syscall
;       [36:5] free named register rax
;       [36:5] free named register rdx
;       [36:5] free named register rsi
;       [36:5] free named register rdi
;       [37:5] # write is a built-in functions writes to a file descriptor
    print_286_5_end:
;   [287:5] label
    loop_287_5:
;       [288:9] counter = counter + 1
;       [288:19] instructions without scratch register 1, with 3
;       [288:19] counter
;       [288:29] counter + 1
        add qword [rbp + 1033], 1
;       [289:9] print_num(counter)
;       [289:9] address of argument 'counter' to parameter 'num'
;       [289:9] allocate scratch register -> r15
        lea r15, [rbp + 1033]
        mov qword [rbp + 1169], r15
;       [289:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1169]
        call print_num
        POP_REGS
;       [290:9] print(colon)
;       [35:6] print(str[] i8)
        print_290_9:
;           [290:9] alias str -> colon
;           [36:5] write(1, address_of(str), array_size_of(str))
;           [36:5] allocate named register rdi
;           [36:11] 1
            mov rdi, 1
;           [36:5] allocate named register rsi
;           [36:14] rsi = address_of(str)
;           [36:14] = expression
;           [36:14] address_of(str)
            lea rsi, [rbp + 61]
;           [36:5] allocate named register rdx
;           [36:31] rdx = array_size_of(str)
;           [36:31] = expression
;           [36:31] array_size_of(str)
            mov rdx, 2
;           [36:5] allocate named register rax
            mov rax, 1
            syscall
;           [36:5] free named register rax
;           [36:5] free named register rdx
;           [36:5] free named register rsi
;           [36:5] free named register rdi
;           [37:5] # write is a built-in functions writes to a file descriptor
        print_290_9_end:
;       [291:9] print(prompt1)
;       [35:6] print(str[] i8)
        print_291_9:
;           [291:9] alias str -> prompt1
;           [36:5] write(1, address_of(str), array_size_of(str))
;           [36:5] allocate named register rdi
;           [36:11] 1
            mov rdi, 1
;           [36:5] allocate named register rsi
;           [36:14] rsi = address_of(str)
;           [36:14] = expression
;           [36:14] address_of(str)
            lea rsi, [rbp + 21]
;           [36:5] allocate named register rdx
;           [36:31] rdx = array_size_of(str)
;           [36:31] = expression
;           [36:31] array_size_of(str)
            mov rdx, 12
;           [36:5] allocate named register rax
            mov rax, 1
            syscall
;           [36:5] free named register rax
;           [36:5] free named register rdx
;           [36:5] free named register rsi
;           [36:5] free named register rdi
;           [37:5] # write is a built-in functions writes to a file descriptor
        print_291_9_end:
;       [292:9] str_in(nm)
;       [72:6] str_in(s str)
        str_in_292_9:
;           [292:9] alias s -> nm
;           [73:5] var nbytes = read(0, address_of(s.data), array_size_of(s.data))
;           [73:9] nbytes: i64 (8 B @ [rbp + 1169])
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
            lea rsi, [rbp + 1042]
;           [73:18] allocate named register rdx
;           [73:46] rdx = array_size_of(s.data)
;           [73:46] = expression
;           [73:46] array_size_of(s.data)
            mov rdx, 127
;           [73:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1169], rax
;           [73:18] free named register rax
;           [73:18] free named register rdx
;           [73:18] free named register rsi
;           [73:18] free named register rdi
;           [74:5] # read is built-in function that operates on file descriptors
;           [75:5] s.len = nbytes - 1
;           [75:13] instructions without scratch register 3, with 3
;           [75:13] nbytes
;           [75:13] allocate scratch register -> r15
            mov r15b, byte [rbp + 1169]
            mov byte [rbp + 1041], r15b
;           [75:13] free scratch register r15
;           [75:22] s.len - 1
            sub byte [rbp + 1041], 1
        str_in_292_9_end:
        if_293_12:
;       [293:12] ? nm.len == 0
;       [293:12] ? nm.len == 0
        cmp_293_12:
        cmp byte [rbp + 1041], 0
        jne if_295_19
        if_293_12_code:
;           [294:13] break
            jmp loop_287_5_end
        jmp if_293_9_end
        if_295_19:
;       [295:19] ? nm.len <= 4
;       [295:19] ? nm.len <= 4
        cmp_295_19:
        cmp byte [rbp + 1041], 4
        jg if_else_293_9
        if_295_19_code:
;           [296:13] print(prompt2)
;           [35:6] print(str[] i8)
            print_296_13:
;               [296:13] alias str -> prompt2
;               [36:5] write(1, address_of(str), array_size_of(str))
;               [36:5] allocate named register rdi
;               [36:11] 1
                mov rdi, 1
;               [36:5] allocate named register rsi
;               [36:14] rsi = address_of(str)
;               [36:14] = expression
;               [36:14] address_of(str)
                lea rsi, [rbp + 33]
;               [36:5] allocate named register rdx
;               [36:31] rdx = array_size_of(str)
;               [36:31] = expression
;               [36:31] array_size_of(str)
                mov rdx, 20
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
;               [37:5] # write is a built-in functions writes to a file descriptor
            print_296_13_end:
;           [297:13] continue
            jmp loop_287_5
        jmp if_293_9_end
        if_else_293_9:
;           [299:13] print(prompt3)
;           [35:6] print(str[] i8)
            print_299_13:
;               [299:13] alias str -> prompt3
;               [36:5] write(1, address_of(str), array_size_of(str))
;               [36:5] allocate named register rdi
;               [36:11] 1
                mov rdi, 1
;               [36:5] allocate named register rsi
;               [36:14] rsi = address_of(str)
;               [36:14] = expression
;               [36:14] address_of(str)
                lea rsi, [rbp + 53]
;               [36:5] allocate named register rdx
;               [36:31] rdx = array_size_of(str)
;               [36:31] = expression
;               [36:31] array_size_of(str)
                mov rdx, 6
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
;               [37:5] # write is a built-in functions writes to a file descriptor
            print_299_13_end:
;           [300:13] str_out(nm)
;           [78:6] str_out(s str)
            str_out_300_13:
;               [300:13] alias s -> nm
;               [79:5] write(1, address_of(s.data), s.len)
;               [79:5] allocate named register rdi
;               [79:11] 1
                mov rdi, 1
;               [79:5] allocate named register rsi
;               [79:14] rsi = address_of(s.data)
;               [79:14] = expression
;               [79:14] address_of(s.data)
                lea rsi, [rbp + 1042]
;               [79:5] allocate named register rdx
;               [79:34] s.len
                movsx rdx, byte [rbp + 1041]
;               [79:5] allocate named register rax
                mov rax, 1
                syscall
;               [79:5] free named register rax
;               [79:5] free named register rdx
;               [79:5] free named register rsi
;               [79:5] free named register rdi
            str_out_300_13_end:
;           [301:13] print(dot)
;           [35:6] print(str[] i8)
            print_301_13:
;               [301:13] alias str -> dot
;               [36:5] write(1, address_of(str), array_size_of(str))
;               [36:5] allocate named register rdi
;               [36:11] 1
                mov rdi, 1
;               [36:5] allocate named register rsi
;               [36:14] rsi = address_of(str)
;               [36:14] = expression
;               [36:14] address_of(str)
                lea rsi, [rbp + 59]
;               [36:5] allocate named register rdx
;               [36:31] rdx = array_size_of(str)
;               [36:31] = expression
;               [36:31] array_size_of(str)
                mov rdx, 1
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
;               [37:5] # write is a built-in functions writes to a file descriptor
            print_301_13_end:
;           [302:13] print(nl)
;           [35:6] print(str[] i8)
            print_302_13:
;               [302:13] alias str -> nl
;               [36:5] write(1, address_of(str), array_size_of(str))
;               [36:5] allocate named register rdi
;               [36:11] 1
                mov rdi, 1
;               [36:5] allocate named register rsi
;               [36:14] rsi = address_of(str)
;               [36:14] = expression
;               [36:14] address_of(str)
                lea rsi, [rbp + 60]
;               [36:5] allocate named register rdx
;               [36:31] rdx = array_size_of(str)
;               [36:31] = expression
;               [36:31] array_size_of(str)
                mov rdx, 1
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
;               [37:5] # write is a built-in functions writes to a file descriptor
            print_302_13_end:
        if_293_9_end:
    jmp loop_287_5
    loop_287_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; 
;[102:15] noinline print_num(num)
print_num:
;   [102:25] num: i64 (8 B @ [rbx])
;   [103:5] var buf[20] i8
;   [103:9] buf: i8[20] (20 B @ [rbx + 8])
;   [103:9] zero 20 * 1 B = 20 B
;   [103:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [104:5] var n = num
;   [104:9] n: i64 (8 B @ [rbx + 28])
;   [104:9] n = num
;   [104:13] num
;   [104:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [104:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
;   [104:13] free scratch register r14
;   [104:13] free scratch register r15
;   [105:5] var is_negative bool = false
;   [105:9] is_negative: bool (1 B @ [rbx + 36])
;   [105:9] is_negative = false
    mov byte [rbx + 36], 0
    if_107_8:
;   [107:8] ? n < 0
;   [107:8] ? n < 0
    cmp_107_8:
    cmp qword [rbx + 28], 0
    jge if_107_5_end
    if_107_8_code:
;       [108:9] is_negative = true
        mov byte [rbx + 36], 1
;       [109:9] n = -n
;       [109:14] -n
        neg qword [rbx + 28]
    if_107_5_end:
;   [112:5] var i = 20
;   [112:9] i: i64 (8 B @ [rbx + 37])
;   [112:9] i = 20
;   [112:13] 20
    mov qword [rbx + 37], 20
;   [113:5] label
    loop_113_5:
;       [114:9] i = i - 1
;       [114:13] instructions without scratch register 1, with 3
;       [114:13] i
;       [114:17] i - 1
        sub qword [rbx + 37], 1
;       [115:9] var ascii = 48 + (n % 10)
;       [115:13] ascii: i64 (8 B @ [rbx + 45])
;       [115:13] ascii = 48 + (n % 10)
;       [115:21] instructions without scratch register 8, with 9
;       [115:21] 48
        mov qword [rbx + 45], 48
;       [115:27] ascii + (n % 10)
;       [115:27] allocate scratch register -> r15
;       [115:27] n
        mov r15, qword [rbx + 28]
;       [115:31] r15 % 10
;       [115:31] div const
;       [115:31] allocate named register rax
        mov rax, r15
;       [115:31] allocate named register rdx
        cqo
;       [115:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [115:31] free scratch register r14
        mov r15, rdx
;       [115:31] free named register rdx
;       [115:31] free named register rax
        add qword [rbx + 45], r15
;       [115:27] free scratch register r15
;       [116:9] # note: not buf[i] = 48 + ... because expression will be executed as
;       [117:9] # byte sized and n overflows
;       [118:9] buf[i] = ascii
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
;       [118:18] ascii
;       [118:18] allocate scratch register -> r14
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
;       [118:18] free scratch register r14
;       [118:9] free scratch register r15
;       [119:9] n = n / 10
;       [119:13] instructions without scratch register 5, with 7
;       [119:13] n
;       [119:17] n / 10
;       [119:17] div const
;       [119:17] allocate named register rax
        mov rax, qword [rbx + 28]
;       [119:17] allocate named register rdx
        cqo
;       [119:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [119:17] free scratch register r15
        mov qword [rbx + 28], rax
;       [119:17] free named register rdx
;       [119:17] free named register rax
        if_120_12:
;       [120:12] ? n == 0
;       [120:12] ? n == 0
        cmp_120_12:
        cmp qword [rbx + 28], 0
        jne if_120_9_end
        if_120_12_code:
;           [120:19] break
            jmp loop_113_5_end
        if_120_9_end:
    jmp loop_113_5
    loop_113_5_end:
    if_123_8:
;   [123:8] ? is_negative
;   [123:8] ? is_negative
    cmp_123_8:
    cmp byte [rbx + 36], 0
    je if_123_5_end
    if_123_8_code:
;       [124:9] i = i - 1
;       [124:13] instructions without scratch register 1, with 3
;       [124:13] i
;       [124:17] i - 1
        sub qword [rbx + 37], 1
;       [125:9] buf[i] = 45
;       [125:9] allocate scratch register -> r15
;       [125:13] set array index
;       [125:13] i
        mov r15, qword [rbx + 37]
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
;       [125:18] 45
        mov byte [rbx + r15 + 8], 45
;       [125:9] free scratch register r15
    if_123_5_end:
;   [128:5] var write_pos = 0
;   [128:9] write_pos: i64 (8 B @ [rbx + 45])
;   [128:9] write_pos = 0
;   [128:21] 0
    mov qword [rbx + 45], 0
;   [129:5] label
    loop_129_5:
;       [130:9] buf[write_pos] = buf[i]
;       [130:9] allocate scratch register -> r15
;       [130:13] set array index
;       [130:13] write_pos
        mov r15, qword [rbx + 45]
;       [130:13] bounds check
;       [130:13] allocate scratch register -> r14
;       [130:13] line number
        mov r14, 130
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [130:13] free scratch register r14
;       [130:26] buf[i]
;       [130:26] allocate scratch register -> r14
;       [130:30] set array index
;       [130:30] i
        mov r14, qword [rbx + 37]
;       [130:30] bounds check
;       [130:30] allocate scratch register -> r13
;       [130:30] line number
        mov r13, 130
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [130:30] free scratch register r13
;       [130:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [130:26] free scratch register r13
;       [130:26] free scratch register r14
;       [130:9] free scratch register r15
;       [131:9] write_pos = write_pos + 1
;       [131:21] instructions without scratch register 1, with 3
;       [131:21] write_pos
;       [131:33] write_pos + 1
        add qword [rbx + 45], 1
;       [132:9] i = i + 1
;       [132:13] instructions without scratch register 1, with 3
;       [132:13] i
;       [132:17] i + 1
        add qword [rbx + 37], 1
        if_133_12:
;       [133:12] ? i == 20
;       [133:12] ? i == 20
        cmp_133_12:
        cmp qword [rbx + 37], 20
        jne if_133_9_end
        if_133_12_code:
;           [133:20] break
            jmp loop_129_5_end
        if_133_9_end:
    jmp loop_129_5
    loop_129_5_end:
;   [136:5] write(1, address_of(buf), write_pos)
;   [136:5] allocate named register rdi
;   [136:11] 1
    mov rdi, 1
;   [136:5] allocate named register rsi
;   [136:14] rsi = address_of(buf)
;   [136:14] = expression
;   [136:14] address_of(buf)
    lea rsi, [rbx + 8]
;   [136:5] allocate named register rdx
;   [136:31] write_pos
    mov rdx, qword [rbx + 45]
;   [136:5] allocate named register rax
    mov rax, 1
    syscall
;   [136:5] free named register rax
;   [136:5] free named register rdx
;   [136:5] free named register rsi
;   [136:5] free named register rdi
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
;                max vars size: 953 B
;          optimization pass 1: 120
;          optimization pass 2: 0
```
