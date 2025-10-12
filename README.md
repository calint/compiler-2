# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super loop program with non-reentrant inlined functions

## Supports

* built-in integer types (64, 32, 16, 8 bit)
* built-in boolean type
* user defined types
* arrays
* optional bounds checking at runtime
  * optional line number
* inlined functions
* keywords: `func`, `type`, `field`, `var`, `loop`, `if`, `else`, `continue`,
`break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`, `address_of`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
run `prog.baz`
  * optional parameters: _stack size_, _bounds check_, with _line number information_ e.g:
    * `./run-baz.sh myprogram.baz --stack=262144`: stack size, no runtime checks
    * `./run-baz.sh myprogram.baz --stack=262144 --check=bounds`: checks bounds without
    line number information (faster)
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=bounds,line`: checks
    bounds with line number information
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    41           1081            621           5308
C++                              1             67             80            489
-------------------------------------------------------------------------------
SUM:                            42           1148            701           5797
-------------------------------------------------------------------------------
```

## Sample

```text
field   hello = "hello world from baz\n"
field prompt1 = "enter name:\n"
field prompt2 = "that is not a name.\n"
field prompt3 = "hello "
field     dot = "."
field      nl = "\n"

# all functions are inlined

# arguments can be placed in specified register using `reg_...` syntax

func exit(v : reg_rdi) {
    mov(rax, 60)  # exit system call
    mov(rdi, v)   # return code
    syscall()
}

func assert(expr : bool) {
    if not expr exit(1)
}

func print(len : reg_rdx, ptr : reg_rsi) {
    mov(rax, 1)   # write system call
    mov(rdi, 1)   # file descriptor for standard out
    mov(rsi, ptr) # buffer address 
    mov(rdx, len) # buffer size
    syscall()
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

func main() {
    var arr : i32[4]
    # arrays are initialized to 0

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

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    ix = 3
    arr[ix] = ~inv(arr[ix - 1])
    assert(arr[ix] == 2)

    var p : point = {0, 0}
    foo(p)
    assert(p.x == 2)
    assert(p.y == 0xb)

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

    var input_buffer : i8[80]

    var worlds : world[8]
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_size_of(worlds[1].locations)
    )
    assert(worlds[0].locations[1] == 0xffee)

    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        
        var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
        # note: `array_size_of` and `address_of` are built-in functions
        #       -1 to not include the trailing '\n'

        if len == 0 {
            break
        } else if len <= 4 {
            print(prompt2.len, prompt2)
            continue
        } else {
            print(prompt3.len, prompt3)
            print(len, address_of(input_buffer))
            print(dot.len, dot)
            print(nl.len, nl)
        }
    }
}
```

## Generates

```nasm
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $-hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '', 10,''
nl.len equ $-nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    cmp r15, 4
    mov r14, 79
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    cmp r15, 4
    mov r14, 80
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    cmp r14, 4
    mov r13, 80
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_81_12:
        mov r13, 1
        cmp r13, 4
        mov r12, 81
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
    assert_81_5:
        if_19_8_81_5:
        cmp_19_8_81_5:
        cmp r15, false
        jne if_19_5_81_5_end
        if_19_8_81_5_code:
            mov rdi, 1
            exit_19_17_81_5:
                mov rax, 60
                syscall
            exit_19_17_81_5_end:
        if_19_5_81_5_end:
    assert_81_5_end:
    cmp_82_12:
        mov r13, 2
        cmp r13, 4
        mov r12, 82
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_82_12
    bool_true_82_12:
    mov r15, true
    jmp bool_end_82_12
    bool_false_82_12:
    mov r15, false
    bool_end_82_12:
    assert_82_5:
        if_19_8_82_5:
        cmp_19_8_82_5:
        cmp r15, false
        jne if_19_5_82_5_end
        if_19_8_82_5_code:
            mov rdi, 1
            exit_19_17_82_5:
                mov rax, 60
                syscall
            exit_19_17_82_5_end:
        if_19_5_82_5_end:
    assert_82_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, rcx
    add r14, r15
    cmp r14, 4
    mov r13, 84
    cmovg rbp, r13
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    cmp rcx, 4
    mov r15, 84
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_86_12:
        mov r13, 0
        cmp r13, 4
        mov r12, 86
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_19_8_86_5:
        cmp_19_8_86_5:
        cmp r15, false
        jne if_19_5_86_5_end
        if_19_8_86_5_code:
            mov rdi, 1
            exit_19_17_86_5:
                mov rax, 60
                syscall
            exit_19_17_86_5_end:
        if_19_5_86_5_end:
    assert_86_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    cmp rcx, 4
    mov r15, 89
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    cmp rcx, 8
    mov r15, 89
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_90_12:
        mov rcx, 4
        cmp rcx, 4
        mov r13, 90
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        mov r13, 90
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_90_12
        mov r14, false
        jmp cmps_end_90_12
        cmps_eq_90_12:
        mov r14, true
        cmps_end_90_12:
    cmp r14, false
    je bool_false_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_19_8_90_5:
        cmp_19_8_90_5:
        cmp r15, false
        jne if_19_5_90_5_end
        if_19_8_90_5_code:
            mov rdi, 1
            exit_19_17_90_5:
                mov rax, 60
                syscall
            exit_19_17_90_5_end:
        if_19_5_90_5_end:
    assert_90_5_end:
    mov r15, 2
    cmp r15, 8
    mov r14, 92
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_93_12:
        mov rcx, 4
        cmp rcx, 4
        mov r13, 93
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        mov r13, 93
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_93_16
        mov r14, false
        jmp cmps_end_93_16
        cmps_eq_93_16:
        mov r14, true
        cmps_end_93_16:
    cmp r14, false
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
    assert_93_5:
        if_19_8_93_5:
        cmp_19_8_93_5:
        cmp r15, false
        jne if_19_5_93_5_end
        if_19_8_93_5_code:
            mov rdi, 1
            exit_19_17_93_5:
                mov rax, 60
                syscall
            exit_19_17_93_5_end:
        if_19_5_93_5_end:
    assert_93_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    cmp r15, 4
    mov r14, 96
    cmovge rbp, r14
    jge panic_bounds
    mov r13, qword [rsp - 24]
    sub r13, 1
    cmp r13, 4
    mov r12, 96
    cmovge rbp, r12
    jge panic_bounds
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_96_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_96_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_97_12:
        mov r13, qword [rsp - 24]
        cmp r13, 4
        mov r12, 97
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_19_8_97_5:
        cmp_19_8_97_5:
        cmp r15, false
        jne if_19_5_97_5_end
        if_19_8_97_5_code:
            mov rdi, 1
            exit_19_17_97_5:
                mov rax, 60
                syscall
            exit_19_17_97_5_end:
        if_19_5_97_5_end:
    assert_97_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_100_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_100_5_end:
    cmp_101_12:
    cmp qword [rsp - 72], 2
    jne bool_false_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_19_8_101_5:
        cmp_19_8_101_5:
        cmp r15, false
        jne if_19_5_101_5_end
        if_19_8_101_5_code:
            mov rdi, 1
            exit_19_17_101_5:
                mov rax, 60
                syscall
            exit_19_17_101_5_end:
        if_19_5_101_5_end:
    assert_101_5_end:
    cmp_102_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_102_12
    bool_true_102_12:
    mov r15, true
    jmp bool_end_102_12
    bool_false_102_12:
    mov r15, false
    bool_end_102_12:
    assert_102_5:
        if_19_8_102_5:
        cmp_19_8_102_5:
        cmp r15, false
        jne if_19_5_102_5_end
        if_19_8_102_5_code:
            mov rdi, 1
            exit_19_17_102_5:
                mov rax, 60
                syscall
            exit_19_17_102_5_end:
        if_19_5_102_5_end:
    assert_102_5_end:
    mov qword [rsp - 80], 0
    bar_105_5:
        if_59_8_105_5:
        cmp_59_8_105_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_105_5_end
        if_59_8_105_5_code:
            jmp bar_105_5_end
        if_59_5_105_5_end:
        mov qword [rsp - 80], 0xff
    bar_105_5_end:
    cmp_106_12:
    cmp qword [rsp - 80], 0
    jne bool_false_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
    assert_106_5:
        if_19_8_106_5:
        cmp_19_8_106_5:
        cmp r15, false
        jne if_19_5_106_5_end
        if_19_8_106_5_code:
            mov rdi, 1
            exit_19_17_106_5:
                mov rax, 60
                syscall
            exit_19_17_106_5_end:
        if_19_5_106_5_end:
    assert_106_5_end:
    mov qword [rsp - 80], 1
    bar_109_5:
        if_59_8_109_5:
        cmp_59_8_109_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_109_5_end
        if_59_8_109_5_code:
            jmp bar_109_5_end
        if_59_5_109_5_end:
        mov qword [rsp - 80], 0xff
    bar_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_19_8_110_5:
        cmp_19_8_110_5:
        cmp r15, false
        jne if_19_5_110_5_end
        if_19_8_110_5_code:
            mov rdi, 1
            exit_19_17_110_5:
                mov rax, 60
                syscall
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
    assert_110_5_end:
    mov qword [rsp - 88], 1
    baz_113_13:
        mov r15, qword [rsp - 88]
        mov qword [rsp - 96], r15
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_113_13_end:
    cmp_114_12:
    cmp qword [rsp - 96], 2
    jne bool_false_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
    assert_114_5:
        if_19_8_114_5:
        cmp_19_8_114_5:
        cmp r15, false
        jne if_19_5_114_5_end
        if_19_8_114_5_code:
            mov rdi, 1
            exit_19_17_114_5:
                mov rax, 60
                syscall
            exit_19_17_114_5_end:
        if_19_5_114_5_end:
    assert_114_5_end:
    baz_116_9:
        mov qword [rsp - 96], 1
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_116_9_end:
    cmp_117_12:
    cmp qword [rsp - 96], 2
    jne bool_false_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_19_8_117_5:
        cmp_19_8_117_5:
        cmp r15, false
        jne if_19_5_117_5_end
        if_19_8_117_5_code:
            mov rdi, 1
            exit_19_17_117_5:
                mov rax, 60
                syscall
            exit_19_17_117_5_end:
        if_19_5_117_5_end:
    assert_117_5_end:
    baz_119_23:
        mov qword [rsp - 112], 2
        mov r15, qword [rsp - 112]
        imul r15, 2
        mov qword [rsp - 112], r15
    baz_119_23_end:
    mov qword [rsp - 104], 0
    cmp_120_12:
    cmp qword [rsp - 112], 4
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_19_8_120_5:
        cmp_19_8_120_5:
        cmp r15, false
        jne if_19_5_120_5_end
        if_19_8_120_5_code:
            mov rdi, 1
            exit_19_17_120_5:
                mov rax, 60
                syscall
            exit_19_17_120_5_end:
        if_19_5_120_5_end:
    assert_120_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 148]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_126_12:
    cmp qword [rsp - 148], 10
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_19_8_126_5:
        cmp_19_8_126_5:
        cmp r15, false
        jne if_19_5_126_5_end
        if_19_8_126_5_code:
            mov rdi, 1
            exit_19_17_126_5:
                mov rax, 60
                syscall
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
    assert_126_5_end:
    cmp_127_12:
    cmp qword [rsp - 140], 2
    jne bool_false_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_19_8_127_5:
        cmp_19_8_127_5:
        cmp r15, false
        jne if_19_5_127_5_end
        if_19_8_127_5_code:
            mov rdi, 1
            exit_19_17_127_5:
                mov rax, 60
                syscall
            exit_19_17_127_5_end:
        if_19_5_127_5_end:
    assert_127_5_end:
    cmp_128_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_128_12
    bool_true_128_12:
    mov r15, true
    jmp bool_end_128_12
    bool_false_128_12:
    mov r15, false
    bool_end_128_12:
    assert_128_5:
        if_19_8_128_5:
        cmp_19_8_128_5:
        cmp r15, false
        jne if_19_5_128_5_end
        if_19_8_128_5_code:
            mov rdi, 1
            exit_19_17_128_5:
                mov rax, 60
                syscall
            exit_19_17_128_5_end:
        if_19_5_128_5_end:
    assert_128_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
    cmp_132_12:
    cmp qword [rsp - 148], -1
    jne bool_false_132_12
    bool_true_132_12:
    mov r15, true
    jmp bool_end_132_12
    bool_false_132_12:
    mov r15, false
    bool_end_132_12:
    assert_132_5:
        if_19_8_132_5:
        cmp_19_8_132_5:
        cmp r15, false
        jne if_19_5_132_5_end
        if_19_8_132_5_code:
            mov rdi, 1
            exit_19_17_132_5:
                mov rax, 60
                syscall
            exit_19_17_132_5_end:
        if_19_5_132_5_end:
    assert_132_5_end:
    cmp_133_12:
    cmp qword [rsp - 140], -2
    jne bool_false_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
    assert_133_5:
        if_19_8_133_5:
        cmp_19_8_133_5:
        cmp r15, false
        jne if_19_5_133_5_end
        if_19_8_133_5_code:
            mov rdi, 1
            exit_19_17_133_5:
                mov rax, 60
                syscall
            exit_19_17_133_5_end:
        if_19_5_133_5_end:
    assert_133_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_136_12:
    cmp qword [rsp - 184], -1
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    cmp_137_12:
    cmp qword [rsp - 176], -2
    jne bool_false_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
    assert_137_5:
        if_19_8_137_5:
        cmp_19_8_137_5:
        cmp r15, false
        jne if_19_5_137_5_end
        if_19_8_137_5_code:
            mov rdi, 1
            exit_19_17_137_5:
                mov rax, 60
                syscall
            exit_19_17_137_5_end:
        if_19_5_137_5_end:
    assert_137_5_end:
    cmp_138_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_138_12
    bool_true_138_12:
    mov r15, true
    jmp bool_end_138_12
    bool_false_138_12:
    mov r15, false
    bool_end_138_12:
    assert_138_5:
        if_19_8_138_5:
        cmp_19_8_138_5:
        cmp r15, false
        jne if_19_5_138_5_end
        if_19_8_138_5_code:
            mov rdi, 1
            exit_19_17_138_5:
                mov rax, 60
                syscall
            exit_19_17_138_5_end:
        if_19_5_138_5_end:
    assert_138_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_143_12:
        lea r13, [rsp - 204]
        mov r12, 0
        cmp r12, 1
        mov r11, 143
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
    assert_143_5:
        if_19_8_143_5:
        cmp_19_8_143_5:
        cmp r15, false
        jne if_19_5_143_5_end
        if_19_8_143_5_code:
            mov rdi, 1
            exit_19_17_143_5:
                mov rax, 60
                syscall
            exit_19_17_143_5_end:
        if_19_5_143_5_end:
    assert_143_5_end:
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 8
    mov r13, 148
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    cmp r14, 8
    mov r13, 148
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8 + 0], 0xffee
    cmp_149_12:
        lea r13, [rsp - 796]
        mov r12, 1
        cmp r12, 8
        mov r11, 149
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 8
        mov r11, 149
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_149_12
    bool_true_149_12:
    mov r15, true
    jmp bool_end_149_12
    bool_false_149_12:
    mov r15, false
    bool_end_149_12:
    assert_149_5:
        if_19_8_149_5:
        cmp_19_8_149_5:
        cmp r15, false
        jne if_19_5_149_5_end
        if_19_8_149_5_code:
            mov rdi, 1
            exit_19_17_149_5:
                mov rax, 60
                syscall
            exit_19_17_149_5_end:
        if_19_5_149_5_end:
    assert_149_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 8
    mov r13, 152
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 8
    mov r14, 152
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    cmp r14, 8
    mov r13, 153
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 8
    mov r14, 153
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_156_12:
        lea r13, [rsp - 796]
        mov r12, 0
        cmp r12, 8
        mov r11, 156
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 8
        mov r11, 156
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_156_12
    bool_true_156_12:
    mov r15, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15, false
    bool_end_156_12:
    assert_156_5:
        if_19_8_156_5:
        cmp_19_8_156_5:
        cmp r15, false
        jne if_19_5_156_5_end
        if_19_8_156_5_code:
            mov rdi, 1
            exit_19_17_156_5:
                mov rax, 60
                syscall
            exit_19_17_156_5_end:
        if_19_5_156_5_end:
    assert_156_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_158_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_158_5_end:
    loop_159_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_160_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_160_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_162_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 804], rax
        read_162_19_end:
        sub qword [rsp - 804], 1
        if_166_12:
        cmp_166_12:
        cmp qword [rsp - 804], 0
        jne if_168_19
        if_166_12_code:
            jmp loop_159_5_end
        jmp if_166_9_end
        if_168_19:
        cmp_168_19:
        cmp qword [rsp - 804], 4
        jg if_else_166_9
        if_168_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_169_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_169_13_end:
            jmp loop_159_5
        jmp if_166_9_end
        if_else_166_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_172_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_172_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_173_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_173_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_174_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_174_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_175_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_175_13_end:
        if_166_9_end:
    jmp loop_159_5
    loop_159_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [rel msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [rel num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
.convert_loop:
    xor rdx, rdx
    mov rcx, 10
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jnz .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    lea rdx, [rel num_buffer + 20]
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
```

## With comments

```nasm

; generated by baz

section .bss
stk resd 131072
stk.end:

true equ 1
false equ 0

section .data
;[1:1] field hello = "hello world from baz\n"
hello: db 'hello world from baz', 10,''
hello.len equ $-hello
;[2:1] field prompt1 = "enter name:\n"
prompt1: db 'enter name:', 10,''
prompt1.len equ $-prompt1
;[3:1] field prompt2 = "that is not a name.\n"
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $-prompt2
;[4:1] field prompt3 = "hello "
prompt3: db 'hello '
prompt3.len equ $-prompt3
;[5:1] field dot = "."
dot: db '.'
dot.len equ $-dot
;[6:1] field nl = "\n"
nl: db '', 10,''
nl.len equ $-nl

section .text
bits 64
global _start
_start:
mov rsp,stk.end
; program
;[8:1] # all functions are inlined
;[10:1] # arguments can be placed in specified register using `reg_...` syntax
;[39:1] # user types are defined using keyword `type`
;[41:1] # default type is `i64` and does not need to be specified
;[49:1] # function arguments are equivalent to mutable references
;[56:1] # default argument type is `i64`
;[63:1] # return target is specified as a variable, in this case `res`
main:
;   [74:5] var arr : i32[4]
;   [74:9] arr: i32[4] @ dword [rsp - 16]
;   [74:9] clear array 4 * 4 B = 16 B
;   [74:5] allocate named register 'rdi'
;   [74:5] allocate named register 'rcx'
;   [74:5] allocate named register 'rax'
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
;   [74:5] free named register 'rax'
;   [74:5] free named register 'rcx'
;   [74:5] free named register 'rdi'
;   [75:5] # arrays are initialized to 0
;   [77:5] var ix = 1
;   [77:9] ix: i64 @ qword [rsp - 24]
;   [77:9] ix =1
;   [77:14] 1
;   [77:14] 1
;   [77:14] ix = 1
    mov qword [rsp - 24], 1
;   [79:5] arr[ix] = 2
;   [79:5] allocate scratch register -> r15
;   [79:9] ix
;   [79:9] ix
;   [79:9] r15 = ix
    mov r15, qword [rsp - 24]
;   [79:9] bounds check
    cmp r15, 4
;   [79:9] allocate scratch register -> r14
;   [79:9] line number
    mov r14, 79
    cmovge rbp, r14
;   [79:9] free scratch register 'r14'
    jge panic_bounds
;   [79:15] 2
;   [79:15] 2
;   [79:15] dword [rsp + r15 * 4 - 16] = 2
    mov dword [rsp + r15 * 4 - 16], 2
;   [79:5] free scratch register 'r15'
;   [80:5] arr[ix + 1] = arr[ix]
;   [80:5] allocate scratch register -> r15
;   [80:9] ix + 1
;   [80:9] ix + 1
;   [80:9] r15 = ix
    mov r15, qword [rsp - 24]
;   [80:14] r15 + 1
    add r15, 1
;   [80:9] bounds check
    cmp r15, 4
;   [80:9] allocate scratch register -> r14
;   [80:9] line number
    mov r14, 80
    cmovge rbp, r14
;   [80:9] free scratch register 'r14'
    jge panic_bounds
;   [80:19] arr[ix]
;   [80:19] arr[ix]
;   [80:19] dword [rsp + r15 * 4 - 16] = arr[ix]
;   [80:19] arr[ix]
;   [80:19] allocate scratch register -> r14
;   [80:23] ix
;   [80:23] ix
;   [80:23] r14 = ix
    mov r14, qword [rsp - 24]
;   [80:23] bounds check
    cmp r14, 4
;   [80:23] allocate scratch register -> r13
;   [80:23] line number
    mov r13, 80
    cmovge rbp, r13
;   [80:23] free scratch register 'r13'
    jge panic_bounds
;   [80:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;   [80:19] free scratch register 'r13'
;   [80:19] free scratch register 'r14'
;   [80:5] free scratch register 'r15'
;   [81:5] assert(arr[1] == 2)
;   [81:12] allocate scratch register -> r15
;   [81:12] arr[1] == 2
;   [81:12] ? arr[1] == 2
;   [81:12] ? arr[1] == 2
    cmp_81_12:
;   [81:12] allocate scratch register -> r14
;       [81:12] arr[1]
;       [81:12] r14 = arr[1]
;       [81:12] arr[1]
;       [81:12] allocate scratch register -> r13
;       [81:16] 1
;       [81:16] 1
;       [81:16] r13 = 1
        mov r13, 1
;       [81:16] bounds check
        cmp r13, 4
;       [81:16] allocate scratch register -> r12
;       [81:16] line number
        mov r12, 81
        cmovge rbp, r12
;       [81:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [81:12] free scratch register 'r13'
    cmp r14, 2
;   [81:12] free scratch register 'r14'
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
;   [18:6] assert(expr : bool) 
    assert_81_5:
;       [81:5] alias expr -> r15
        if_19_8_81_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_81_5:
        cmp r15, false
        jne if_19_5_81_5_end
        if_19_8_81_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_81_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_81_5_end:
        if_19_5_81_5_end:
;       [81:5] free scratch register 'r15'
    assert_81_5_end:
;   [82:5] assert(arr[2] == 2)
;   [82:12] allocate scratch register -> r15
;   [82:12] arr[2] == 2
;   [82:12] ? arr[2] == 2
;   [82:12] ? arr[2] == 2
    cmp_82_12:
;   [82:12] allocate scratch register -> r14
;       [82:12] arr[2]
;       [82:12] r14 = arr[2]
;       [82:12] arr[2]
;       [82:12] allocate scratch register -> r13
;       [82:16] 2
;       [82:16] 2
;       [82:16] r13 = 2
        mov r13, 2
;       [82:16] bounds check
        cmp r13, 4
;       [82:16] allocate scratch register -> r12
;       [82:16] line number
        mov r12, 82
        cmovge rbp, r12
;       [82:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [82:12] free scratch register 'r13'
    cmp r14, 2
;   [82:12] free scratch register 'r14'
    jne bool_false_82_12
    bool_true_82_12:
    mov r15, true
    jmp bool_end_82_12
    bool_false_82_12:
    mov r15, false
    bool_end_82_12:
;   [18:6] assert(expr : bool) 
    assert_82_5:
;       [82:5] alias expr -> r15
        if_19_8_82_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_82_5:
        cmp r15, false
        jne if_19_5_82_5_end
        if_19_8_82_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_82_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_82_5_end:
        if_19_5_82_5_end:
;       [82:5] free scratch register 'r15'
    assert_82_5_end:
;   [84:5] array_copy(arr[2], arr, 2)
;   [84:5] allocate named register 'rsi'
;   [84:5] allocate named register 'rdi'
;   [84:5] allocate named register 'rcx'
;   [84:29] 2
;   [84:29] 2
;   [84:29] 2
;   [84:29] rcx = 2
    mov rcx, 2
;   [84:16] arr[2]
;   [84:16] allocate scratch register -> r15
;   [84:20] 2
;   [84:20] 2
;   [84:20] r15 = 2
    mov r15, 2
;   [84:20] bounds check
;   [84:20] allocate scratch register -> r14
    mov r14, rcx
    add r14, r15
    cmp r14, 4
;   [84:20] allocate scratch register -> r13
;   [84:20] line number
    mov r13, 84
    cmovg rbp, r13
;   [84:20] free scratch register 'r13'
;   [84:20] free scratch register 'r14'
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
;   [84:5] free scratch register 'r15'
;   [84:24] arr
;   [84:24] bounds check
    cmp rcx, 4
;   [84:24] allocate scratch register -> r15
;   [84:24] line number
    mov r15, 84
    cmovg rbp, r15
;   [84:24] free scratch register 'r15'
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
;   [84:5] free named register 'rcx'
;   [84:5] free named register 'rdi'
;   [84:5] free named register 'rsi'
;   [85:5] # copy from, to, number of elements
;   [86:5] assert(arr[0] == 2)
;   [86:12] allocate scratch register -> r15
;   [86:12] arr[0] == 2
;   [86:12] ? arr[0] == 2
;   [86:12] ? arr[0] == 2
    cmp_86_12:
;   [86:12] allocate scratch register -> r14
;       [86:12] arr[0]
;       [86:12] r14 = arr[0]
;       [86:12] arr[0]
;       [86:12] allocate scratch register -> r13
;       [86:16] 0
;       [86:16] 0
;       [86:16] r13 = 0
        mov r13, 0
;       [86:16] bounds check
        cmp r13, 4
;       [86:16] allocate scratch register -> r12
;       [86:16] line number
        mov r12, 86
        cmovge rbp, r12
;       [86:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [86:12] free scratch register 'r13'
    cmp r14, 2
;   [86:12] free scratch register 'r14'
    jne bool_false_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
;   [18:6] assert(expr : bool) 
    assert_86_5:
;       [86:5] alias expr -> r15
        if_19_8_86_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_86_5:
        cmp r15, false
        jne if_19_5_86_5_end
        if_19_8_86_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_86_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_86_5_end:
        if_19_5_86_5_end:
;       [86:5] free scratch register 'r15'
    assert_86_5_end:
;   [88:5] var arr1 : i32[8]
;   [88:9] arr1: i32[8] @ dword [rsp - 56]
;   [88:9] clear array 8 * 4 B = 32 B
;   [88:5] allocate named register 'rdi'
;   [88:5] allocate named register 'rcx'
;   [88:5] allocate named register 'rax'
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
;   [88:5] free named register 'rax'
;   [88:5] free named register 'rcx'
;   [88:5] free named register 'rdi'
;   [89:5] array_copy(arr, arr1, 4)
;   [89:5] allocate named register 'rsi'
;   [89:5] allocate named register 'rdi'
;   [89:5] allocate named register 'rcx'
;   [89:27] 4
;   [89:27] 4
;   [89:27] 4
;   [89:27] rcx = 4
    mov rcx, 4
;   [89:16] arr
;   [89:16] bounds check
    cmp rcx, 4
;   [89:16] allocate scratch register -> r15
;   [89:16] line number
    mov r15, 89
    cmovg rbp, r15
;   [89:16] free scratch register 'r15'
    jg panic_bounds
    lea rsi, [rsp - 16]
;   [89:21] arr1
;   [89:21] bounds check
    cmp rcx, 8
;   [89:21] allocate scratch register -> r15
;   [89:21] line number
    mov r15, 89
    cmovg rbp, r15
;   [89:21] free scratch register 'r15'
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
;   [89:5] free named register 'rcx'
;   [89:5] free named register 'rdi'
;   [89:5] free named register 'rsi'
;   [90:5] assert(arrays_equal(arr, arr1, 4))
;   [90:12] allocate scratch register -> r15
;   [90:12] arrays_equal(arr, arr1, 4)
;   [90:12] ? arrays_equal(arr, arr1, 4)
;   [90:12] ? arrays_equal(arr, arr1, 4)
    cmp_90_12:
;   [90:12] allocate scratch register -> r14
;       [90:12] arrays_equal(arr, arr1, 4)
;       [90:12] r14 = arrays_equal(arr, arr1, 4)
;       [90:12] arrays_equal(arr, arr1, 4)
;       [90:12] allocate named register 'rsi'
;       [90:12] allocate named register 'rdi'
;       [90:12] allocate named register 'rcx'
;       [90:36] 4
;       [90:36] 4
;       [90:36] 4
;       [90:36] rcx = 4
        mov rcx, 4
;       [90:25] arr
;       [90:25] bounds check
        cmp rcx, 4
;       [90:25] allocate scratch register -> r13
;       [90:25] line number
        mov r13, 90
        cmovg rbp, r13
;       [90:25] free scratch register 'r13'
        jg panic_bounds
        lea rsi, [rsp - 16]
;       [90:30] arr1
;       [90:30] bounds check
        cmp rcx, 8
;       [90:30] allocate scratch register -> r13
;       [90:30] line number
        mov r13, 90
        cmovg rbp, r13
;       [90:30] free scratch register 'r13'
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [90:12] free named register 'rcx'
;       [90:12] free named register 'rdi'
;       [90:12] free named register 'rsi'
        je cmps_eq_90_12
        mov r14, false
        jmp cmps_end_90_12
        cmps_eq_90_12:
        mov r14, true
        cmps_end_90_12:
    cmp r14, false
;   [90:12] free scratch register 'r14'
    je bool_false_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
;   [18:6] assert(expr : bool) 
    assert_90_5:
;       [90:5] alias expr -> r15
        if_19_8_90_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_90_5:
        cmp r15, false
        jne if_19_5_90_5_end
        if_19_8_90_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_90_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_90_5_end:
        if_19_5_90_5_end:
;       [90:5] free scratch register 'r15'
    assert_90_5_end:
;   [92:5] arr1[2] = -1
;   [92:5] allocate scratch register -> r15
;   [92:10] 2
;   [92:10] 2
;   [92:10] r15 = 2
    mov r15, 2
;   [92:10] bounds check
    cmp r15, 8
;   [92:10] allocate scratch register -> r14
;   [92:10] line number
    mov r14, 92
    cmovge rbp, r14
;   [92:10] free scratch register 'r14'
    jge panic_bounds
;   [92:15] -1
;   [92:15] -1
;   [92:16] dword [rsp + r15 * 4 - 56] = -1
    mov dword [rsp + r15 * 4 - 56], -1
;   [92:5] free scratch register 'r15'
;   [93:5] assert(not arrays_equal(arr, arr1, 4))
;   [93:12] allocate scratch register -> r15
;   [93:12] not arrays_equal(arr, arr1, 4)
;   [93:12] ? not arrays_equal(arr, arr1, 4)
;   [93:12] ? not arrays_equal(arr, arr1, 4)
    cmp_93_12:
;   [93:16] allocate scratch register -> r14
;       [93:16] arrays_equal(arr, arr1, 4)
;       [93:16] r14 = arrays_equal(arr, arr1, 4)
;       [93:16] arrays_equal(arr, arr1, 4)
;       [93:16] allocate named register 'rsi'
;       [93:16] allocate named register 'rdi'
;       [93:16] allocate named register 'rcx'
;       [93:40] 4
;       [93:40] 4
;       [93:40] 4
;       [93:40] rcx = 4
        mov rcx, 4
;       [93:29] arr
;       [93:29] bounds check
        cmp rcx, 4
;       [93:29] allocate scratch register -> r13
;       [93:29] line number
        mov r13, 93
        cmovg rbp, r13
;       [93:29] free scratch register 'r13'
        jg panic_bounds
        lea rsi, [rsp - 16]
;       [93:34] arr1
;       [93:34] bounds check
        cmp rcx, 8
;       [93:34] allocate scratch register -> r13
;       [93:34] line number
        mov r13, 93
        cmovg rbp, r13
;       [93:34] free scratch register 'r13'
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [93:16] free named register 'rcx'
;       [93:16] free named register 'rdi'
;       [93:16] free named register 'rsi'
        je cmps_eq_93_16
        mov r14, false
        jmp cmps_end_93_16
        cmps_eq_93_16:
        mov r14, true
        cmps_end_93_16:
    cmp r14, false
;   [93:12] free scratch register 'r14'
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
;   [18:6] assert(expr : bool) 
    assert_93_5:
;       [93:5] alias expr -> r15
        if_19_8_93_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_93_5:
        cmp r15, false
        jne if_19_5_93_5_end
        if_19_8_93_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_93_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_93_5_end:
        if_19_5_93_5_end:
;       [93:5] free scratch register 'r15'
    assert_93_5_end:
;   [95:5] ix = 3
;   [95:10] 3
;   [95:10] 3
;   [95:10] ix = 3
    mov qword [rsp - 24], 3
;   [96:5] arr[ix] = ~inv(arr[ix - 1])
;   [96:5] allocate scratch register -> r15
;   [96:9] ix
;   [96:9] ix
;   [96:9] r15 = ix
    mov r15, qword [rsp - 24]
;   [96:9] bounds check
    cmp r15, 4
;   [96:9] allocate scratch register -> r14
;   [96:9] line number
    mov r14, 96
    cmovge rbp, r14
;   [96:9] free scratch register 'r14'
    jge panic_bounds
;   [96:15] ~inv(arr[ix - 1])
;   [96:15] ~inv(arr[ix - 1])
;   [96:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;   [96:16] ~inv(arr[ix - 1])
;   [96:20] allocate scratch register -> r14
;   [96:20] arr[ix - 1]
;   [96:20] arr[ix - 1]
;   [96:20] r14 = arr[ix - 1]
;   [96:20] arr[ix - 1]
;   [96:20] allocate scratch register -> r13
;   [96:24] ix - 1
;   [96:24] ix - 1
;   [96:24] r13 = ix
    mov r13, qword [rsp - 24]
;   [96:29] r13 - 1
    sub r13, 1
;   [96:24] bounds check
    cmp r13, 4
;   [96:24] allocate scratch register -> r12
;   [96:24] line number
    mov r12, 96
    cmovge rbp, r12
;   [96:24] free scratch register 'r12'
    jge panic_bounds
    movsx r14, dword [rsp + r13 * 4 - 16]
;   [96:20] free scratch register 'r13'
;   [65:6] inv(i : i32) : i32 res 
    inv_96_16:
;       [96:16] alias res -> dword [rsp + r15 * 4 - 16]
;       [96:16] alias i -> r14
;       [66:5] res = ~i
;       [66:11] ~i
;       [66:11] ~i
;       [66:12] res = ~i
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
;       [96:16] free scratch register 'r14'
    inv_96_16_end:
    not dword [rsp + r15 * 4 - 16]
;   [96:5] free scratch register 'r15'
;   [97:5] assert(arr[ix] == 2)
;   [97:12] allocate scratch register -> r15
;   [97:12] arr[ix] == 2
;   [97:12] ? arr[ix] == 2
;   [97:12] ? arr[ix] == 2
    cmp_97_12:
;   [97:12] allocate scratch register -> r14
;       [97:12] arr[ix]
;       [97:12] r14 = arr[ix]
;       [97:12] arr[ix]
;       [97:12] allocate scratch register -> r13
;       [97:16] ix
;       [97:16] ix
;       [97:16] r13 = ix
        mov r13, qword [rsp - 24]
;       [97:16] bounds check
        cmp r13, 4
;       [97:16] allocate scratch register -> r12
;       [97:16] line number
        mov r12, 97
        cmovge rbp, r12
;       [97:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [97:12] free scratch register 'r13'
    cmp r14, 2
;   [97:12] free scratch register 'r14'
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
;   [18:6] assert(expr : bool) 
    assert_97_5:
;       [97:5] alias expr -> r15
        if_19_8_97_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_97_5:
        cmp r15, false
        jne if_19_5_97_5_end
        if_19_8_97_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_97_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_97_5_end:
        if_19_5_97_5_end:
;       [97:5] free scratch register 'r15'
    assert_97_5_end:
;   [99:5] var p : point = {0, 0}
;   [99:9] p: point @ qword [rsp - 72]
;   [99:9] p ={0, 0}
;   [99:21] {0, 0}
;   [99:21] {0, 0}
;   [99:22] 0
;   [99:22] 0
;   [99:22] p.x = 0
    mov qword [rsp - 72], 0
;   [99:25] 0
;   [99:25] 0
;   [99:25] p.y = 0
    mov qword [rsp - 64], 0
;   [100:5] foo(p)
;   [51:6] foo(pt : point) 
    foo_100_5:
;       [100:5] alias pt -> p
;       [52:5] pt.x = 0b10
;       [52:12] 0b10
;       [52:12] 0b10
;       [52:12] pt.x = 0b10
        mov qword [rsp - 72], 0b10
;       [52:20] # binary value 2
;       [53:5] pt.y = 0xb
;       [53:12] 0xb
;       [53:12] 0xb
;       [53:12] pt.y = 0xb
        mov qword [rsp - 64], 0xb
;       [53:20] # hex value 11
    foo_100_5_end:
;   [101:5] assert(p.x == 2)
;   [101:12] allocate scratch register -> r15
;   [101:12] p.x == 2
;   [101:12] ? p.x == 2
;   [101:12] ? p.x == 2
    cmp_101_12:
    cmp qword [rsp - 72], 2
    jne bool_false_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
;   [18:6] assert(expr : bool) 
    assert_101_5:
;       [101:5] alias expr -> r15
        if_19_8_101_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_101_5:
        cmp r15, false
        jne if_19_5_101_5_end
        if_19_8_101_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_101_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_101_5_end:
        if_19_5_101_5_end:
;       [101:5] free scratch register 'r15'
    assert_101_5_end:
;   [102:5] assert(p.y == 0xb)
;   [102:12] allocate scratch register -> r15
;   [102:12] p.y == 0xb
;   [102:12] ? p.y == 0xb
;   [102:12] ? p.y == 0xb
    cmp_102_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_102_12
    bool_true_102_12:
    mov r15, true
    jmp bool_end_102_12
    bool_false_102_12:
    mov r15, false
    bool_end_102_12:
;   [18:6] assert(expr : bool) 
    assert_102_5:
;       [102:5] alias expr -> r15
        if_19_8_102_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_102_5:
        cmp r15, false
        jne if_19_5_102_5_end
        if_19_8_102_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_102_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_102_5_end:
        if_19_5_102_5_end:
;       [102:5] free scratch register 'r15'
    assert_102_5_end:
;   [104:5] var i = 0
;   [104:9] i: i64 @ qword [rsp - 80]
;   [104:9] i =0
;   [104:13] 0
;   [104:13] 0
;   [104:13] i = 0
    mov qword [rsp - 80], 0
;   [105:5] bar(i)
;   [58:6] bar(arg) 
    bar_105_5:
;       [105:5] alias arg -> i
        if_59_8_105_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_105_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_105_5_end
        if_59_8_105_5_code:
;           [59:17] return
            jmp bar_105_5_end
        if_59_5_105_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
;       [60:11] 0xff
;       [60:11] arg = 0xff
        mov qword [rsp - 80], 0xff
    bar_105_5_end:
;   [106:5] assert(i == 0)
;   [106:12] allocate scratch register -> r15
;   [106:12] i == 0
;   [106:12] ? i == 0
;   [106:12] ? i == 0
    cmp_106_12:
    cmp qword [rsp - 80], 0
    jne bool_false_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
;   [18:6] assert(expr : bool) 
    assert_106_5:
;       [106:5] alias expr -> r15
        if_19_8_106_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_106_5:
        cmp r15, false
        jne if_19_5_106_5_end
        if_19_8_106_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_106_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_106_5_end:
        if_19_5_106_5_end:
;       [106:5] free scratch register 'r15'
    assert_106_5_end:
;   [108:5] i = 1
;   [108:9] 1
;   [108:9] 1
;   [108:9] i = 1
    mov qword [rsp - 80], 1
;   [109:5] bar(i)
;   [58:6] bar(arg) 
    bar_109_5:
;       [109:5] alias arg -> i
        if_59_8_109_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_109_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_109_5_end
        if_59_8_109_5_code:
;           [59:17] return
            jmp bar_109_5_end
        if_59_5_109_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
;       [60:11] 0xff
;       [60:11] arg = 0xff
        mov qword [rsp - 80], 0xff
    bar_109_5_end:
;   [110:5] assert(i == 0xff)
;   [110:12] allocate scratch register -> r15
;   [110:12] i == 0xff
;   [110:12] ? i == 0xff
;   [110:12] ? i == 0xff
    cmp_110_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
;   [18:6] assert(expr : bool) 
    assert_110_5:
;       [110:5] alias expr -> r15
        if_19_8_110_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_110_5:
        cmp r15, false
        jne if_19_5_110_5_end
        if_19_8_110_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_110_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
;       [110:5] free scratch register 'r15'
    assert_110_5_end:
;   [112:5] var j = 1
;   [112:9] j: i64 @ qword [rsp - 88]
;   [112:9] j =1
;   [112:13] 1
;   [112:13] 1
;   [112:13] j = 1
    mov qword [rsp - 88], 1
;   [113:5] var k = baz(j)
;   [113:9] k: i64 @ qword [rsp - 96]
;   [113:9] k =baz(j)
;   [113:13] baz(j)
;   [113:13] baz(j)
;   [113:13] k = baz(j)
;   [113:13] baz(j)
;   [69:6] baz(arg) : i64 res 
    baz_113_13:
;       [113:13] alias res -> k
;       [113:13] alias arg -> j
;       [70:5] res = arg * 2
;       [70:11] arg * 2
;       [70:11] arg * 2
;       [70:11] res = arg
;       [70:11] allocate scratch register -> r15
        mov r15, qword [rsp - 88]
        mov qword [rsp - 96], r15
;       [70:11] free scratch register 'r15'
;       [70:17] res * 2
;       [70:17] allocate scratch register -> r15
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
;       [70:17] free scratch register 'r15'
    baz_113_13_end:
;   [114:5] assert(k == 2)
;   [114:12] allocate scratch register -> r15
;   [114:12] k == 2
;   [114:12] ? k == 2
;   [114:12] ? k == 2
    cmp_114_12:
    cmp qword [rsp - 96], 2
    jne bool_false_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
;   [18:6] assert(expr : bool) 
    assert_114_5:
;       [114:5] alias expr -> r15
        if_19_8_114_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_114_5:
        cmp r15, false
        jne if_19_5_114_5_end
        if_19_8_114_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_114_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_114_5_end:
        if_19_5_114_5_end:
;       [114:5] free scratch register 'r15'
    assert_114_5_end:
;   [116:5] k = baz(1)
;   [116:9] baz(1)
;   [116:9] baz(1)
;   [116:9] k = baz(1)
;   [116:9] baz(1)
;   [69:6] baz(arg) : i64 res 
    baz_116_9:
;       [116:9] alias res -> k
;       [116:9] alias arg -> 1
;       [70:5] res = arg * 2
;       [70:11] arg * 2
;       [70:11] arg * 2
;       [70:11] res = arg
        mov qword [rsp - 96], 1
;       [70:17] res * 2
;       [70:17] allocate scratch register -> r15
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
;       [70:17] free scratch register 'r15'
    baz_116_9_end:
;   [117:5] assert(k == 2)
;   [117:12] allocate scratch register -> r15
;   [117:12] k == 2
;   [117:12] ? k == 2
;   [117:12] ? k == 2
    cmp_117_12:
    cmp qword [rsp - 96], 2
    jne bool_false_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
;   [18:6] assert(expr : bool) 
    assert_117_5:
;       [117:5] alias expr -> r15
        if_19_8_117_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_117_5:
        cmp r15, false
        jne if_19_5_117_5_end
        if_19_8_117_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_117_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_117_5_end:
        if_19_5_117_5_end:
;       [117:5] free scratch register 'r15'
    assert_117_5_end:
;   [119:5] var p0 : point = {baz(2), 0}
;   [119:9] p0: point @ qword [rsp - 112]
;   [119:9] p0 ={baz(2), 0}
;   [119:22] {baz(2), 0}
;   [119:22] {baz(2), 0}
;   [119:23] baz(2)
;   [119:23] baz(2)
;   [119:23] p0.x = baz(2)
;   [119:23] baz(2)
;   [69:6] baz(arg) : i64 res 
    baz_119_23:
;       [119:23] alias res -> p0.x
;       [119:23] alias arg -> 2
;       [70:5] res = arg * 2
;       [70:11] arg * 2
;       [70:11] arg * 2
;       [70:11] res = arg
        mov qword [rsp - 112], 2
;       [70:17] res * 2
;       [70:17] allocate scratch register -> r15
        mov r15, qword [rsp - 112]
        imul r15, 2
        mov qword [rsp - 112], r15
;       [70:17] free scratch register 'r15'
    baz_119_23_end:
;   [119:31] 0
;   [119:31] 0
;   [119:31] p0.y = 0
    mov qword [rsp - 104], 0
;   [120:5] assert(p0.x == 4)
;   [120:12] allocate scratch register -> r15
;   [120:12] p0.x == 4
;   [120:12] ? p0.x == 4
;   [120:12] ? p0.x == 4
    cmp_120_12:
    cmp qword [rsp - 112], 4
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
;   [18:6] assert(expr : bool) 
    assert_120_5:
;       [120:5] alias expr -> r15
        if_19_8_120_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_120_5:
        cmp r15, false
        jne if_19_5_120_5_end
        if_19_8_120_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_120_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_120_5_end:
        if_19_5_120_5_end:
;       [120:5] free scratch register 'r15'
    assert_120_5_end:
;   [122:5] var x = 1
;   [122:9] x: i64 @ qword [rsp - 120]
;   [122:9] x =1
;   [122:13] 1
;   [122:13] 1
;   [122:13] x = 1
    mov qword [rsp - 120], 1
;   [123:5] var y = 2
;   [123:9] y: i64 @ qword [rsp - 128]
;   [123:9] y =2
;   [123:13] 2
;   [123:13] 2
;   [123:13] y = 2
    mov qword [rsp - 128], 2
;   [125:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [125:9] o1: object @ qword [rsp - 148]
;   [125:9] o1 ={{x * 10, y}, 0xff0000}
;   [125:23] {{x * 10, y}, 0xff0000}
;   [125:23] {{x * 10, y}, 0xff0000}
;       [125:24] {x * 10, y}
;       [125:25] x * 10
;       [125:25] x * 10
;       [125:25] o1.pos.x = x
;       [125:25] allocate scratch register -> r15
        mov r15, qword [rsp - 120]
        mov qword [rsp - 148], r15
;       [125:25] free scratch register 'r15'
;       [125:29] o1.pos.x * 10
;       [125:29] allocate scratch register -> r15
        mov r15, qword [rsp - 148]
        imul r15, 10
        mov qword [rsp - 148], r15
;       [125:29] free scratch register 'r15'
;       [125:33] y
;       [125:33] y
;       [125:33] o1.pos.y = y
;       [125:33] allocate scratch register -> r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
;       [125:33] free scratch register 'r15'
;   [125:37] 0xff0000
;   [125:37] 0xff0000
;   [125:37] o1.color = 0xff0000
    mov dword [rsp - 132], 0xff0000
;   [126:5] assert(o1.pos.x == 10)
;   [126:12] allocate scratch register -> r15
;   [126:12] o1.pos.x == 10
;   [126:12] ? o1.pos.x == 10
;   [126:12] ? o1.pos.x == 10
    cmp_126_12:
    cmp qword [rsp - 148], 10
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
;   [18:6] assert(expr : bool) 
    assert_126_5:
;       [126:5] alias expr -> r15
        if_19_8_126_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_126_5:
        cmp r15, false
        jne if_19_5_126_5_end
        if_19_8_126_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_126_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
;       [126:5] free scratch register 'r15'
    assert_126_5_end:
;   [127:5] assert(o1.pos.y == 2)
;   [127:12] allocate scratch register -> r15
;   [127:12] o1.pos.y == 2
;   [127:12] ? o1.pos.y == 2
;   [127:12] ? o1.pos.y == 2
    cmp_127_12:
    cmp qword [rsp - 140], 2
    jne bool_false_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
;   [18:6] assert(expr : bool) 
    assert_127_5:
;       [127:5] alias expr -> r15
        if_19_8_127_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_127_5:
        cmp r15, false
        jne if_19_5_127_5_end
        if_19_8_127_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_127_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_127_5_end:
        if_19_5_127_5_end:
;       [127:5] free scratch register 'r15'
    assert_127_5_end:
;   [128:5] assert(o1.color == 0xff0000)
;   [128:12] allocate scratch register -> r15
;   [128:12] o1.color == 0xff0000
;   [128:12] ? o1.color == 0xff0000
;   [128:12] ? o1.color == 0xff0000
    cmp_128_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_128_12
    bool_true_128_12:
    mov r15, true
    jmp bool_end_128_12
    bool_false_128_12:
    mov r15, false
    bool_end_128_12:
;   [18:6] assert(expr : bool) 
    assert_128_5:
;       [128:5] alias expr -> r15
        if_19_8_128_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_128_5:
        cmp r15, false
        jne if_19_5_128_5_end
        if_19_8_128_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_128_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_128_5_end:
        if_19_5_128_5_end:
;       [128:5] free scratch register 'r15'
    assert_128_5_end:
;   [130:5] var p1 : point = {-x, -y}
;   [130:9] p1: point @ qword [rsp - 164]
;   [130:9] p1 ={-x, -y}
;   [130:22] {-x, -y}
;   [130:22] {-x, -y}
;   [130:23] -x
;   [130:23] -x
;   [130:24] p1.x = -x
;   [130:24] allocate scratch register -> r15
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
;   [130:24] free scratch register 'r15'
    neg qword [rsp - 164]
;   [130:27] -y
;   [130:27] -y
;   [130:28] p1.y = -y
;   [130:28] allocate scratch register -> r15
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
;   [130:28] free scratch register 'r15'
    neg qword [rsp - 156]
;   [131:5] o1.pos = p1
;   [131:5] allocate named register 'rsi'
;   [131:5] allocate named register 'rdi'
;   [131:5] allocate named register 'rcx'
    lea rdi, [rsp - 148]
;   [131:14] p1
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
;   [131:5] free named register 'rcx'
;   [131:5] free named register 'rdi'
;   [131:5] free named register 'rsi'
;   [132:5] assert(o1.pos.x == -1)
;   [132:12] allocate scratch register -> r15
;   [132:12] o1.pos.x == -1
;   [132:12] ? o1.pos.x == -1
;   [132:12] ? o1.pos.x == -1
    cmp_132_12:
    cmp qword [rsp - 148], -1
    jne bool_false_132_12
    bool_true_132_12:
    mov r15, true
    jmp bool_end_132_12
    bool_false_132_12:
    mov r15, false
    bool_end_132_12:
;   [18:6] assert(expr : bool) 
    assert_132_5:
;       [132:5] alias expr -> r15
        if_19_8_132_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_132_5:
        cmp r15, false
        jne if_19_5_132_5_end
        if_19_8_132_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_132_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_132_5_end:
        if_19_5_132_5_end:
;       [132:5] free scratch register 'r15'
    assert_132_5_end:
;   [133:5] assert(o1.pos.y == -2)
;   [133:12] allocate scratch register -> r15
;   [133:12] o1.pos.y == -2
;   [133:12] ? o1.pos.y == -2
;   [133:12] ? o1.pos.y == -2
    cmp_133_12:
    cmp qword [rsp - 140], -2
    jne bool_false_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
;   [18:6] assert(expr : bool) 
    assert_133_5:
;       [133:5] alias expr -> r15
        if_19_8_133_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_133_5:
        cmp r15, false
        jne if_19_5_133_5_end
        if_19_8_133_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_133_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_133_5_end:
        if_19_5_133_5_end:
;       [133:5] free scratch register 'r15'
    assert_133_5_end:
;   [135:5] var o2 : object = o1
;   [135:9] o2: object @ qword [rsp - 184]
;   [135:9] o2 =o1
;   [135:9] allocate named register 'rsi'
;   [135:9] allocate named register 'rdi'
;   [135:9] allocate named register 'rcx'
    lea rdi, [rsp - 184]
;   [135:23] o1
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
;   [135:9] free named register 'rcx'
;   [135:9] free named register 'rdi'
;   [135:9] free named register 'rsi'
;   [136:5] assert(o2.pos.x == -1)
;   [136:12] allocate scratch register -> r15
;   [136:12] o2.pos.x == -1
;   [136:12] ? o2.pos.x == -1
;   [136:12] ? o2.pos.x == -1
    cmp_136_12:
    cmp qword [rsp - 184], -1
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
;   [18:6] assert(expr : bool) 
    assert_136_5:
;       [136:5] alias expr -> r15
        if_19_8_136_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_136_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
;       [136:5] free scratch register 'r15'
    assert_136_5_end:
;   [137:5] assert(o2.pos.y == -2)
;   [137:12] allocate scratch register -> r15
;   [137:12] o2.pos.y == -2
;   [137:12] ? o2.pos.y == -2
;   [137:12] ? o2.pos.y == -2
    cmp_137_12:
    cmp qword [rsp - 176], -2
    jne bool_false_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
;   [18:6] assert(expr : bool) 
    assert_137_5:
;       [137:5] alias expr -> r15
        if_19_8_137_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_137_5:
        cmp r15, false
        jne if_19_5_137_5_end
        if_19_8_137_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_137_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_137_5_end:
        if_19_5_137_5_end:
;       [137:5] free scratch register 'r15'
    assert_137_5_end:
;   [138:5] assert(o2.color == 0xff0000)
;   [138:12] allocate scratch register -> r15
;   [138:12] o2.color == 0xff0000
;   [138:12] ? o2.color == 0xff0000
;   [138:12] ? o2.color == 0xff0000
    cmp_138_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_138_12
    bool_true_138_12:
    mov r15, true
    jmp bool_end_138_12
    bool_false_138_12:
    mov r15, false
    bool_end_138_12:
;   [18:6] assert(expr : bool) 
    assert_138_5:
;       [138:5] alias expr -> r15
        if_19_8_138_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_138_5:
        cmp r15, false
        jne if_19_5_138_5_end
        if_19_8_138_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_138_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_138_5_end:
        if_19_5_138_5_end:
;       [138:5] free scratch register 'r15'
    assert_138_5_end:
;   [140:5] var o3 : object[1]
;   [140:9] o3: object[1] @ qword [rsp - 204]
;   [140:9] clear array 1 * 20 B = 20 B
;   [140:5] allocate named register 'rdi'
;   [140:5] allocate named register 'rcx'
;   [140:5] allocate named register 'rax'
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
;   [140:5] free named register 'rax'
;   [140:5] free named register 'rcx'
;   [140:5] free named register 'rdi'
;   [141:5] # index 0 in an array can be accessed without array index
;   [142:5] o3.pos.y = 73
;   [142:16] 73
;   [142:16] 73
;   [142:16] o3.pos.y = 73
    mov qword [rsp - 196], 73
;   [143:5] assert(o3[0].pos.y == 73)
;   [143:12] allocate scratch register -> r15
;   [143:12] o3[0].pos.y == 73
;   [143:12] ? o3[0].pos.y == 73
;   [143:12] ? o3[0].pos.y == 73
    cmp_143_12:
;   [143:12] allocate scratch register -> r14
;       [143:12] o3[0].pos.y
;       [143:12] r14 = o3[0].pos.y
;       [143:12] o3[0].pos.y
;       [143:12] allocate scratch register -> r13
        lea r13, [rsp - 204]
;       [143:12] allocate scratch register -> r12
;       [143:15] 0
;       [143:15] 0
;       [143:15] r12 = 0
        mov r12, 0
;       [143:15] bounds check
        cmp r12, 1
;       [143:15] allocate scratch register -> r11
;       [143:15] line number
        mov r11, 143
        cmovge rbp, r11
;       [143:15] free scratch register 'r11'
        jge panic_bounds
        imul r12, 20
        add r13, r12
;       [143:12] free scratch register 'r12'
        add r13, 8
        mov r14, qword [r13]
;       [143:12] free scratch register 'r13'
    cmp r14, 73
;   [143:12] free scratch register 'r14'
    jne bool_false_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
;   [18:6] assert(expr : bool) 
    assert_143_5:
;       [143:5] alias expr -> r15
        if_19_8_143_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_143_5:
        cmp r15, false
        jne if_19_5_143_5_end
        if_19_8_143_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_143_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_143_5_end:
        if_19_5_143_5_end:
;       [143:5] free scratch register 'r15'
    assert_143_5_end:
;   [145:5] var input_buffer : i8[80]
;   [145:9] input_buffer: i8[80] @ byte [rsp - 284]
;   [145:9] clear array 80 * 1 B = 80 B
;   [145:5] allocate named register 'rdi'
;   [145:5] allocate named register 'rcx'
;   [145:5] allocate named register 'rax'
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
;   [145:5] free named register 'rax'
;   [145:5] free named register 'rcx'
;   [145:5] free named register 'rdi'
;   [147:5] var worlds : world[8]
;   [147:9] worlds: world[8] @ qword [rsp - 796]
;   [147:9] clear array 8 * 64 B = 512 B
;   [147:5] allocate named register 'rdi'
;   [147:5] allocate named register 'rcx'
;   [147:5] allocate named register 'rax'
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
;   [147:5] free named register 'rax'
;   [147:5] free named register 'rcx'
;   [147:5] free named register 'rdi'
;   [148:5] worlds[1].locations[1] = 0xffee
;   [148:5] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [148:5] allocate scratch register -> r14
;   [148:12] 1
;   [148:12] 1
;   [148:12] r14 = 1
    mov r14, 1
;   [148:12] bounds check
    cmp r14, 8
;   [148:12] allocate scratch register -> r13
;   [148:12] line number
    mov r13, 148
    cmovge rbp, r13
;   [148:12] free scratch register 'r13'
    jge panic_bounds
    shl r14, 6
    add r15, r14
;   [148:5] free scratch register 'r14'
;   [148:5] allocate scratch register -> r14
;   [148:25] 1
;   [148:25] 1
;   [148:25] r14 = 1
    mov r14, 1
;   [148:25] bounds check
    cmp r14, 8
;   [148:25] allocate scratch register -> r13
;   [148:25] line number
    mov r13, 148
    cmovge rbp, r13
;   [148:25] free scratch register 'r13'
    jge panic_bounds
;   [148:30] 0xffee
;   [148:30] 0xffee
;   [148:30] qword [r15 + r14 * 8 + 0] = 0xffee
    mov qword [r15 + r14 * 8 + 0], 0xffee
;   [148:5] free scratch register 'r14'
;   [148:5] free scratch register 'r15'
;   [149:5] assert(worlds[1].locations[1] == 0xffee)
;   [149:12] allocate scratch register -> r15
;   [149:12] worlds[1].locations[1] == 0xffee
;   [149:12] ? worlds[1].locations[1] == 0xffee
;   [149:12] ? worlds[1].locations[1] == 0xffee
    cmp_149_12:
;   [149:12] allocate scratch register -> r14
;       [149:12] worlds[1].locations[1]
;       [149:12] r14 = worlds[1].locations[1]
;       [149:12] worlds[1].locations[1]
;       [149:12] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [149:12] allocate scratch register -> r12
;       [149:19] 1
;       [149:19] 1
;       [149:19] r12 = 1
        mov r12, 1
;       [149:19] bounds check
        cmp r12, 8
;       [149:19] allocate scratch register -> r11
;       [149:19] line number
        mov r11, 149
        cmovge rbp, r11
;       [149:19] free scratch register 'r11'
        jge panic_bounds
        shl r12, 6
        add r13, r12
;       [149:12] free scratch register 'r12'
;       [149:12] allocate scratch register -> r12
;       [149:32] 1
;       [149:32] 1
;       [149:32] r12 = 1
        mov r12, 1
;       [149:32] bounds check
        cmp r12, 8
;       [149:32] allocate scratch register -> r11
;       [149:32] line number
        mov r11, 149
        cmovge rbp, r11
;       [149:32] free scratch register 'r11'
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
;       [149:12] free scratch register 'r12'
;       [149:12] free scratch register 'r13'
    cmp r14, 0xffee
;   [149:12] free scratch register 'r14'
    jne bool_false_149_12
    bool_true_149_12:
    mov r15, true
    jmp bool_end_149_12
    bool_false_149_12:
    mov r15, false
    bool_end_149_12:
;   [18:6] assert(expr : bool) 
    assert_149_5:
;       [149:5] alias expr -> r15
        if_19_8_149_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_149_5:
        cmp r15, false
        jne if_19_5_149_5_end
        if_19_8_149_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_149_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_149_5_end:
        if_19_5_149_5_end:
;       [149:5] free scratch register 'r15'
    assert_149_5_end:
;   [151:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds[1].locations) )
;   [151:5] allocate named register 'rsi'
;   [151:5] allocate named register 'rdi'
;   [151:5] allocate named register 'rcx'
;   [153:9] array_size_of(worlds[1].locations)
;   [153:9] array_size_of(worlds[1].locations)
;   [154:9] array_size_of(worlds[1].locations)
;   [154:9] rcx = array_size_of(worlds[1].locations)
;   [154:9] array_size_of(worlds[1].locations)
    mov rcx, 8
;   [152:9] worlds[1].locations
;   [152:9] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [152:9] allocate scratch register -> r14
;   [152:16] 1
;   [152:16] 1
;   [152:16] r14 = 1
    mov r14, 1
;   [152:16] bounds check
    cmp r14, 8
;   [152:16] allocate scratch register -> r13
;   [152:16] line number
    mov r13, 152
    cmovge rbp, r13
;   [152:16] free scratch register 'r13'
    jge panic_bounds
    shl r14, 6
    add r15, r14
;   [152:9] free scratch register 'r14'
;   [152:9] bounds check
    cmp rcx, 8
;   [152:9] allocate scratch register -> r14
;   [152:9] line number
    mov r14, 152
    cmovg rbp, r14
;   [152:9] free scratch register 'r14'
    jg panic_bounds
    lea rsi, [r15]
;   [151:5] free scratch register 'r15'
;   [153:9] worlds[0].locations
;   [153:9] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [153:9] allocate scratch register -> r14
;   [153:16] 0
;   [153:16] 0
;   [153:16] r14 = 0
    mov r14, 0
;   [153:16] bounds check
    cmp r14, 8
;   [153:16] allocate scratch register -> r13
;   [153:16] line number
    mov r13, 153
    cmovge rbp, r13
;   [153:16] free scratch register 'r13'
    jge panic_bounds
    shl r14, 6
    add r15, r14
;   [153:9] free scratch register 'r14'
;   [153:9] bounds check
    cmp rcx, 8
;   [153:9] allocate scratch register -> r14
;   [153:9] line number
    mov r14, 153
    cmovg rbp, r14
;   [153:9] free scratch register 'r14'
    jg panic_bounds
    lea rdi, [r15]
;   [151:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [151:5] free named register 'rcx'
;   [151:5] free named register 'rdi'
;   [151:5] free named register 'rsi'
;   [156:5] assert(worlds[0].locations[1] == 0xffee)
;   [156:12] allocate scratch register -> r15
;   [156:12] worlds[0].locations[1] == 0xffee
;   [156:12] ? worlds[0].locations[1] == 0xffee
;   [156:12] ? worlds[0].locations[1] == 0xffee
    cmp_156_12:
;   [156:12] allocate scratch register -> r14
;       [156:12] worlds[0].locations[1]
;       [156:12] r14 = worlds[0].locations[1]
;       [156:12] worlds[0].locations[1]
;       [156:12] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [156:12] allocate scratch register -> r12
;       [156:19] 0
;       [156:19] 0
;       [156:19] r12 = 0
        mov r12, 0
;       [156:19] bounds check
        cmp r12, 8
;       [156:19] allocate scratch register -> r11
;       [156:19] line number
        mov r11, 156
        cmovge rbp, r11
;       [156:19] free scratch register 'r11'
        jge panic_bounds
        shl r12, 6
        add r13, r12
;       [156:12] free scratch register 'r12'
;       [156:12] allocate scratch register -> r12
;       [156:32] 1
;       [156:32] 1
;       [156:32] r12 = 1
        mov r12, 1
;       [156:32] bounds check
        cmp r12, 8
;       [156:32] allocate scratch register -> r11
;       [156:32] line number
        mov r11, 156
        cmovge rbp, r11
;       [156:32] free scratch register 'r11'
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
;       [156:12] free scratch register 'r12'
;       [156:12] free scratch register 'r13'
    cmp r14, 0xffee
;   [156:12] free scratch register 'r14'
    jne bool_false_156_12
    bool_true_156_12:
    mov r15, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15, false
    bool_end_156_12:
;   [18:6] assert(expr : bool) 
    assert_156_5:
;       [156:5] alias expr -> r15
        if_19_8_156_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_156_5:
        cmp r15, false
        jne if_19_5_156_5_end
        if_19_8_156_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_156_5:
;               [19:17] alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_156_5_end:
        if_19_5_156_5_end:
;       [156:5] free scratch register 'r15'
    assert_156_5_end:
;   [158:5] print(hello.len, hello)
;   [158:11] allocate named register 'rdx'
    mov rdx, hello.len
;   [158:22] allocate named register 'rsi'
    mov rsi, hello
;   [22:6] print(len : reg_rdx, ptr : reg_rsi) 
    print_158_5:
;       [158:5] alias len -> rdx
;       [158:5] alias ptr -> rsi
;       [23:5] mov(rax, 1)
        mov rax, 1
;       [23:19] # write system call
;       [24:5] mov(rdi, 1)
        mov rdi, 1
;       [24:19] # file descriptor for standard out
;       [25:5] mov(rsi, ptr)
;       [25:19] # buffer address
;       [26:5] mov(rdx, len)
;       [26:19] # buffer size
;       [27:5] syscall()
        syscall
;       [158:5] free named register 'rsi'
;       [158:5] free named register 'rdx'
    print_158_5_end:
;   [159:5] loop
    loop_159_5:
;       [160:9] print(prompt1.len, prompt1)
;       [160:15] allocate named register 'rdx'
        mov rdx, prompt1.len
;       [160:28] allocate named register 'rsi'
        mov rsi, prompt1
;       [22:6] print(len : reg_rdx, ptr : reg_rsi) 
        print_160_9:
;           [160:9] alias len -> rdx
;           [160:9] alias ptr -> rsi
;           [23:5] mov(rax, 1)
            mov rax, 1
;           [23:19] # write system call
;           [24:5] mov(rdi, 1)
            mov rdi, 1
;           [24:19] # file descriptor for standard out
;           [25:5] mov(rsi, ptr)
;           [25:19] # buffer address
;           [26:5] mov(rdx, len)
;           [26:19] # buffer size
;           [27:5] syscall()
            syscall
;           [160:9] free named register 'rsi'
;           [160:9] free named register 'rdx'
        print_160_9_end:
;       [162:9] var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [162:13] len: i64 @ qword [rsp - 804]
;       [162:13] len =read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [162:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [162:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [162:19] len = read(array_size_of(input_buffer), address_of(input_buffer))
;       [162:19] read(array_size_of(input_buffer), address_of(input_buffer))
;       [162:24] allocate named register 'rdx'
;       [162:24] array_size_of(input_buffer)
;       [162:24] array_size_of(input_buffer)
;       [162:24] rdx = array_size_of(input_buffer)
;       [162:24] array_size_of(input_buffer)
        mov rdx, 80
;       [162:53] allocate named register 'rsi'
;       [162:53] address_of(input_buffer)
;       [162:53] address_of(input_buffer)
;       [162:53] rsi = address_of(input_buffer)
;       [162:53] address_of(input_buffer)
        lea rsi, [rsp - 284]
;       [30:6] read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_162_19:
;           [162:19] alias nbytes -> len
;           [162:19] alias len -> rdx
;           [162:19] alias ptr -> rsi
;           [31:5] mov(rax, 0)
            mov rax, 0
;           [31:19] # read system call
;           [32:5] mov(rdi, 0)
            mov rdi, 0
;           [32:19] # file descriptor for standard input
;           [33:5] mov(rsi, ptr)
;           [33:19] # buffer address
;           [34:5] mov(rdx, len)
;           [34:19] # buffer size
;           [35:5] syscall()
            syscall
;           [36:5] mov(nbytes, rax)
            mov qword [rsp - 804], rax
;           [36:22] # return value
;           [162:19] free named register 'rsi'
;           [162:19] free named register 'rdx'
        read_162_19_end:
;       [162:81] len - 1
        sub qword [rsp - 804], 1
;       [163:9] # note: `array_size_of` and `address_of` are built-in functions
;       [164:9] # -1 to not include the trailing '\n'
        if_166_12:
;       [166:12] ? len == 0
;       [166:12] ? len == 0
        cmp_166_12:
        cmp qword [rsp - 804], 0
        jne if_168_19
        if_166_12_code:
;           [167:13] break
            jmp loop_159_5_end
        jmp if_166_9_end
        if_168_19:
;       [168:19] ? len <= 4
;       [168:19] ? len <= 4
        cmp_168_19:
        cmp qword [rsp - 804], 4
        jg if_else_166_9
        if_168_19_code:
;           [169:13] print(prompt2.len, prompt2)
;           [169:19] allocate named register 'rdx'
            mov rdx, prompt2.len
;           [169:32] allocate named register 'rsi'
            mov rsi, prompt2
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_169_13:
;               [169:13] alias len -> rdx
;               [169:13] alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [169:13] free named register 'rsi'
;               [169:13] free named register 'rdx'
            print_169_13_end:
;           [170:13] continue
            jmp loop_159_5
        jmp if_166_9_end
        if_else_166_9:
;           [172:13] print(prompt3.len, prompt3)
;           [172:19] allocate named register 'rdx'
            mov rdx, prompt3.len
;           [172:32] allocate named register 'rsi'
            mov rsi, prompt3
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_172_13:
;               [172:13] alias len -> rdx
;               [172:13] alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [172:13] free named register 'rsi'
;               [172:13] free named register 'rdx'
            print_172_13_end:
;           [173:13] print(len, address_of(input_buffer))
;           [173:19] allocate named register 'rdx'
            mov rdx, qword [rsp - 804]
;           [173:24] allocate named register 'rsi'
;           [173:24] address_of(input_buffer)
;           [173:24] address_of(input_buffer)
;           [173:24] rsi = address_of(input_buffer)
;           [173:24] address_of(input_buffer)
            lea rsi, [rsp - 284]
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_173_13:
;               [173:13] alias len -> rdx
;               [173:13] alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [173:13] free named register 'rsi'
;               [173:13] free named register 'rdx'
            print_173_13_end:
;           [174:13] print(dot.len, dot)
;           [174:19] allocate named register 'rdx'
            mov rdx, dot.len
;           [174:28] allocate named register 'rsi'
            mov rsi, dot
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_174_13:
;               [174:13] alias len -> rdx
;               [174:13] alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [174:13] free named register 'rsi'
;               [174:13] free named register 'rdx'
            print_174_13_end:
;           [175:13] print(nl.len, nl)
;           [175:19] allocate named register 'rdx'
            mov rdx, nl.len
;           [175:27] allocate named register 'rsi'
            mov rsi, nl
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_175_13:
;               [175:13] alias len -> rdx
;               [175:13] alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [175:13] free named register 'rsi'
;               [175:13] free named register 'rdx'
            print_175_13_end:
        if_166_9_end:
    jmp loop_159_5
    loop_159_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

panic_bounds:
;   print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [rel msg_panic]
    mov rdx, msg_panic_len
    syscall
;   line number is in `rbp`
    mov rax, rbp
;   convert to string
    lea rdi, [rel num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
.convert_loop:
    xor rdx, rdx
    mov rcx, 10
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
    lea rdx, [rel num_buffer + 20]
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

; max scratch registers in use: 5
;            max frames in use: 7
;               max stack size: 804 B
```
