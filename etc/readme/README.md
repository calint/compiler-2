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
C/C++ Header                    52           3118            985          11564
C++                              1             46              7            235
-------------------------------------------------------------------------------
SUM:                            53           3164            992          11799
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
.option norvc
.option norelax
.text
.globl _start
_start:
    la s0, dat
main:
    sw zero, 208(s0)
    sw zero, 212(s0)
    sw zero, 216(s0)
    sw zero, 220(s0)
    sw zero, 224(s0)
    cmp_139_12:
    lw t6, 224(s0)
    sltiu t6, t6, 1
    1:
    bool_end_139_12:
    assert_139_5:
        if_34_26_139_5:
        cmp_34_26_139_5:
        bne t6, zero, if_34_23_139_5_end
        1:
        if_34_26_139_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_139_5_end:
    assert_139_5_end:
    li t6, -1
    sw t6, 224(s0)
    cmp_143_12:
    lw t6, 224(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_143_12:
    assert_143_5:
        if_34_26_143_5:
        cmp_34_26_143_5:
        bne t6, zero, if_34_23_143_5_end
        1:
        if_34_26_143_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_143_5_end:
    assert_143_5_end:
        cmp_147_16:
        bool_end_147_16:
        li t6, 1
        assert_147_9:
            if_34_26_147_9:
            cmp_34_26_147_9:
            bne t6, zero, if_34_23_147_9_end
            1:
            if_34_26_147_9_code:
                li a0, 1
                li a7, 93
                ecall
            if_34_23_147_9_end:
        assert_147_9_end:
    cmp_150_12:
    bool_end_150_12:
    li t6, 1
    assert_150_5:
        if_34_26_150_5:
        cmp_34_26_150_5:
        bne t6, zero, if_34_23_150_5_end
        1:
        if_34_26_150_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_150_5_end:
    assert_150_5_end:
    li t6, 1
    sw t6, 228(s0)
    addi t6, s0, 208
    lw t5, 228(s0)
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    li t5, 2
    sw t5, 0(t6)
    addi t6, s0, 208
    lw t5, 228(s0)
    addi t5, t5, 1
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    addi t5, s0, 208
    lw t4, 228(s0)
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t4, 0(t5)
    sw t4, 0(t6)
    cmp_157_12:
    addi t5, s0, 208
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_157_12:
    assert_157_5:
        if_34_26_157_5:
        cmp_34_26_157_5:
        bne t6, zero, if_34_23_157_5_end
        1:
        if_34_26_157_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_157_5_end:
    assert_157_5_end:
    cmp_158_12:
    addi t5, s0, 208
    li t4, 2
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_158_12:
    assert_158_5:
        if_34_26_158_5:
        cmp_34_26_158_5:
        bne t6, zero, if_34_23_158_5_end
        1:
        if_34_26_158_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_158_5_end:
    assert_158_5_end:
    li t6, 2
    addi t3, s0, 208
    li t2, 2
    slli t2, t2, ((2) & 31)
    add t3, t3, t2
    addi t5, t3, 0
    addi t4, s0, 208
    slli t6, t6, ((2) & 31)
    srli t2, t6, 2
    andi t6, t6, 3
    beqz t2, 2f
    1:
    lw t3, 0(t5)
    sw t3, 0(t4)
    addi t5, t5, 4
    addi t4, t4, 4
    addi t2, t2, -1
    bnez t2, 1b
    2:
    andi t3, t6, 2
    beqz t3, 3f
    lhu t3, 0(t5)
    sh t3, 0(t4)
    addi t5, t5, 2
    addi t4, t4, 2
    3:
    andi t6, t6, 1
    beqz t6, 4f
    lbu t3, 0(t5)
    sb t3, 0(t4)
    4:
    cmp_162_12:
    addi t5, s0, 208
    li t4, 0
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_162_12:
    assert_162_5:
        if_34_26_162_5:
        cmp_34_26_162_5:
        bne t6, zero, if_34_23_162_5_end
        1:
        if_34_26_162_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_162_5_end:
    assert_162_5_end:
    addi t6, s0, 232
    li t5, 8
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    li t6, 4
    addi t5, s0, 208
    addi t4, s0, 232
    slli t6, t6, ((2) & 31)
    srli t2, t6, 2
    andi t6, t6, 3
    beqz t2, 2f
    1:
    lw t3, 0(t5)
    sw t3, 0(t4)
    addi t5, t5, 4
    addi t4, t4, 4
    addi t2, t2, -1
    bnez t2, 1b
    2:
    andi t3, t6, 2
    beqz t3, 3f
    lhu t3, 0(t5)
    sh t3, 0(t4)
    addi t5, t5, 2
    addi t4, t4, 2
    3:
    andi t6, t6, 1
    beqz t6, 4f
    lbu t3, 0(t5)
    sb t3, 0(t4)
    4:
    cmp_166_12:
        li t4, 4
        addi t3, s0, 208
        addi t2, s0, 232
        slli t4, t4, ((2) & 31)
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltu t6, zero, t5
    1:
    bool_end_166_12:
    assert_166_5:
        if_34_26_166_5:
        cmp_34_26_166_5:
        bne t6, zero, if_34_23_166_5_end
        1:
        if_34_26_166_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_166_5_end:
    assert_166_5_end:
    addi t6, s0, 232
    li t5, 2
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    li t5, -1
    sw t5, 0(t6)
    cmp_170_12:
        li t4, 4
        addi t3, s0, 208
        addi t2, s0, 232
        slli t4, t4, ((2) & 31)
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltiu t6, t5, 1
    1:
    bool_end_170_12:
    assert_170_5:
        if_34_26_170_5:
        cmp_34_26_170_5:
        bne t6, zero, if_34_23_170_5_end
        1:
        if_34_26_170_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_170_5_end:
    assert_170_5_end:
    li t6, 3
    sw t6, 228(s0)
    addi t6, s0, 208
    lw t5, 228(s0)
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    addi t5, s0, 208
    lw t4, 228(s0)
    addi t4, t4, -1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    inv_173_16:
        lw t4, 0(t5)
        sw t4, 0(t6)
        lw t4, 0(t6)
        xori t4, t4, -1
        sw t4, 0(t6)
    inv_173_16_end:
    lw t5, 0(t6)
    xori t5, t5, -1
    sw t5, 0(t6)
    cmp_174_12:
    addi t5, s0, 208
    lw t4, 228(s0)
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_174_12:
    assert_174_5:
        if_34_26_174_5:
        cmp_34_26_174_5:
        bne t6, zero, if_34_23_174_5_end
        1:
        if_34_26_174_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_174_5_end:
    assert_174_5_end:
    faz_176_5:
        addi t6, s0, 208
        li t5, 1
        slli t5, t5, ((2) & 31)
        add t6, t6, t5
        li t5, 254
        sw t5, 0(t6)
    faz_176_5_end:
    cmp_177_12:
    addi t5, s0, 208
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 254
    sltiu t6, t6, 1
    1:
    bool_end_177_12:
    assert_177_5:
        if_34_26_177_5:
        cmp_34_26_177_5:
        bne t6, zero, if_34_23_177_5_end
        1:
        if_34_26_177_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_177_5_end:
    assert_177_5_end:
    li t6, 3
    sw t6, 264(s0)
    li t6, 5
    sw t6, 268(s0)
    addi t6, s0, 264
    sw zero, 276(s0)
    foo_180_5:
        lw t5, 0(t6)
        lw t4, 276(s0)
        add t5, t5, t4
        addi t5, t5, 2
        sw t5, 0(t6)
        foo_180_5_continue:
            addi t6, t6, 4
            lw t5, 276(s0)
            addi t5, t5, 1
            sw t5, 276(s0)
            li t4, 2
            bne t5, t4, foo_180_5
            1:
    foo_180_5_end:
    cmp_183_12:
    addi t5, s0, 264
    li t4, 0
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
        li t4, 3
        addi t4, t4, 2
    lw t6, 0(t5)
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_183_12:
    assert_183_5:
        if_34_26_183_5:
        cmp_34_26_183_5:
        bne t6, zero, if_34_23_183_5_end
        1:
        if_34_26_183_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_183_5_end:
    assert_183_5_end:
    cmp_184_12:
    addi t5, s0, 264
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
        li t4, 5
        addi t4, t4, 1
        addi t4, t4, 2
    lw t6, 0(t5)
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_184_12:
    assert_184_5:
        if_34_26_184_5:
        cmp_34_26_184_5:
        bne t6, zero, if_34_23_184_5_end
        1:
        if_34_26_184_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_184_5_end:
    assert_184_5_end:
    sw zero, 272(s0)
    sw zero, 276(s0)
    fooz_191_5:
        li t6, 2
        sw t6, 272(s0)
        li t6, 11
        sw t6, 276(s0)
    fooz_191_5_end:
    cmp_192_12:
    lw t6, 272(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_192_12:
    assert_192_5:
        if_34_26_192_5:
        cmp_34_26_192_5:
        bne t6, zero, if_34_23_192_5_end
        1:
        if_34_26_192_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_192_5_end:
    assert_192_5_end:
    cmp_193_12:
    lw t6, 276(s0)
    xori t6, t6, 11
    sltiu t6, t6, 1
    1:
    bool_end_193_12:
    assert_193_5:
        if_34_26_193_5:
        cmp_34_26_193_5:
        bne t6, zero, if_34_23_193_5_end
        1:
        if_34_26_193_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_193_5_end:
    assert_193_5_end:
    lw t6, 272(s0)
    sw t6, 280(s0)
    lw t6, 276(s0)
    sw t6, 284(s0)
    cmp_196_12:
        addi t3, s0, 272
        addi t2, s0, 280
        li t4, 8
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltu t6, zero, t5
    1:
    bool_end_196_12:
    assert_196_5:
        if_34_26_196_5:
        cmp_34_26_196_5:
        bne t6, zero, if_34_23_196_5_end
        1:
        if_34_26_196_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_196_5_end:
    assert_196_5_end:
    li t6, 3
    sw t6, 280(s0)
    cmp_201_12:
        addi t3, s0, 272
        addi t2, s0, 280
        li t4, 8
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltiu t6, t5, 1
    1:
    bool_end_201_12:
    assert_201_5:
        if_34_26_201_5:
        cmp_34_26_201_5:
        bne t6, zero, if_34_23_201_5_end
        1:
        if_34_26_201_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_201_5_end:
    assert_201_5_end:
    sw zero, 288(s0)
    bar_204_5:
        if_50_8_204_5:
        cmp_50_8_204_5:
        lw t6, 288(s0)
        bne t6, zero, if_50_5_204_5_end
        1:
        if_50_8_204_5_code:
            j bar_204_5_end
        if_50_5_204_5_end:
        li t6, 255
        sw t6, 288(s0)
    bar_204_5_end:
    cmp_205_12:
    lw t6, 288(s0)
    sltiu t6, t6, 1
    1:
    bool_end_205_12:
    assert_205_5:
        if_34_26_205_5:
        cmp_34_26_205_5:
        bne t6, zero, if_34_23_205_5_end
        1:
        if_34_26_205_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_205_5_end:
    assert_205_5_end:
    li t6, 1
    sw t6, 288(s0)
    bar_208_5:
        if_50_8_208_5:
        cmp_50_8_208_5:
        lw t6, 288(s0)
        bne t6, zero, if_50_5_208_5_end
        1:
        if_50_8_208_5_code:
            j bar_208_5_end
        if_50_5_208_5_end:
        li t6, 255
        sw t6, 288(s0)
    bar_208_5_end:
    cmp_209_12:
    lw t6, 288(s0)
    xori t6, t6, 255
    sltiu t6, t6, 1
    1:
    bool_end_209_12:
    assert_209_5:
        if_34_26_209_5:
        cmp_34_26_209_5:
        bne t6, zero, if_34_23_209_5_end
        1:
        if_34_26_209_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_209_5_end:
    assert_209_5_end:
    li t6, 1
    sw t6, 292(s0)
    baz_212_13:
        lw t6, 292(s0)
        slli t6, t6, ((1) & 31)
        sw t6, 296(s0)
    baz_212_13_end:
    cmp_213_12:
    lw t6, 296(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_213_12:
    assert_213_5:
        if_34_26_213_5:
        cmp_34_26_213_5:
        bne t6, zero, if_34_23_213_5_end
        1:
        if_34_26_213_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_213_5_end:
    assert_213_5_end:
    baz_215_9:
        li t6, 1
        slli t6, t6, ((1) & 31)
        sw t6, 296(s0)
    baz_215_9_end:
    cmp_216_12:
    lw t6, 296(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_216_12:
    assert_216_5:
        if_34_26_216_5:
        cmp_34_26_216_5:
        bne t6, zero, if_34_23_216_5_end
        1:
        if_34_26_216_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_216_5_end:
    assert_216_5_end:
    baz_218_23:
        li t6, 3
        slli t6, t6, ((1) & 31)
        sw t6, 300(s0)
    baz_218_23_end:
    sw zero, 304(s0)
    cmp_219_12:
    lw t6, 300(s0)
    xori t6, t6, 6
    sltiu t6, t6, 1
    1:
    bool_end_219_12:
    assert_219_5:
        if_34_26_219_5:
        cmp_34_26_219_5:
        bne t6, zero, if_34_23_219_5_end
        1:
        if_34_26_219_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_219_5_end:
    assert_219_5_end:
    point_init_221_22:
        li t6, -1
        sw t6, 308(s0)
        li t6, -2
        sw t6, 312(s0)
    point_init_221_22_end:
    cmp_222_12:
    lw t6, 308(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_222_12:
    assert_222_5:
        if_34_26_222_5:
        cmp_34_26_222_5:
        bne t6, zero, if_34_23_222_5_end
        1:
        if_34_26_222_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_222_5_end:
    assert_222_5_end:
    cmp_223_12:
    lw t6, 312(s0)
    xori t6, t6, -2
    sltiu t6, t6, 1
    1:
    bool_end_223_12:
    assert_223_5:
        if_34_26_223_5:
        cmp_34_26_223_5:
        bne t6, zero, if_34_23_223_5_end
        1:
        if_34_26_223_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_223_5_end:
    assert_223_5_end:
    li t6, 1
    sw t6, 316(s0)
    li t6, 2
    sw t6, 320(s0)
    lw t6, 316(s0)
    addi t5, t6, 0
    slli t6, t5, 2
    add t6, t6, t5
    slli t6, t6, 1
    sw t6, 324(s0)
    lw t6, 320(s0)
    sw t6, 328(s0)
    li t6, 16711680
    sw t6, 332(s0)
    cmp_229_12:
    lw t6, 324(s0)
    xori t6, t6, 10
    sltiu t6, t6, 1
    1:
    bool_end_229_12:
    assert_229_5:
        if_34_26_229_5:
        cmp_34_26_229_5:
        bne t6, zero, if_34_23_229_5_end
        1:
        if_34_26_229_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    lw t6, 328(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_230_12:
    assert_230_5:
        if_34_26_230_5:
        cmp_34_26_230_5:
        bne t6, zero, if_34_23_230_5_end
        1:
        if_34_26_230_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_230_5_end:
    assert_230_5_end:
    cmp_231_12:
    lw t6, 332(s0)
    li t5, 16711680
    xor t6, t6, t5
    sltiu t6, t6, 1
    1:
    bool_end_231_12:
    assert_231_5:
        if_34_26_231_5:
        cmp_34_26_231_5:
        bne t6, zero, if_34_23_231_5_end
        1:
        if_34_26_231_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_231_5_end:
    assert_231_5_end:
    lw t6, 316(s0)
    sw t6, 336(s0)
    lw t6, 336(s0)
    sub t6, zero, t6
    sw t6, 336(s0)
    lw t6, 320(s0)
    sw t6, 340(s0)
    lw t6, 340(s0)
    sub t6, zero, t6
    sw t6, 340(s0)
    lw t6, 336(s0)
    sw t6, 324(s0)
    lw t6, 340(s0)
    sw t6, 328(s0)
    cmp_235_12:
    lw t6, 324(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_235_12:
    assert_235_5:
        if_34_26_235_5:
        cmp_34_26_235_5:
        bne t6, zero, if_34_23_235_5_end
        1:
        if_34_26_235_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_235_5_end:
    assert_235_5_end:
    cmp_236_12:
    lw t6, 328(s0)
    xori t6, t6, -2
    sltiu t6, t6, 1
    1:
    bool_end_236_12:
    assert_236_5:
        if_34_26_236_5:
        cmp_34_26_236_5:
        bne t6, zero, if_34_23_236_5_end
        1:
        if_34_26_236_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_236_5_end:
    assert_236_5_end:
    lw t6, 324(s0)
    sw t6, 344(s0)
    lw t6, 328(s0)
    sw t6, 348(s0)
    lw t6, 332(s0)
    sw t6, 352(s0)
    cmp_239_12:
    lw t6, 344(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_239_12:
    assert_239_5:
        if_34_26_239_5:
        cmp_34_26_239_5:
        bne t6, zero, if_34_23_239_5_end
        1:
        if_34_26_239_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_239_5_end:
    assert_239_5_end:
    cmp_240_12:
    lw t6, 348(s0)
    xori t6, t6, -2
    sltiu t6, t6, 1
    1:
    bool_end_240_12:
    assert_240_5:
        if_34_26_240_5:
        cmp_34_26_240_5:
        bne t6, zero, if_34_23_240_5_end
        1:
        if_34_26_240_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_240_5_end:
    assert_240_5_end:
    cmp_241_12:
    lw t6, 352(s0)
    li t5, 16711680
    xor t6, t6, t5
    sltiu t6, t6, 1
    1:
    bool_end_241_12:
    assert_241_5:
        if_34_26_241_5:
        cmp_34_26_241_5:
        bne t6, zero, if_34_23_241_5_end
        1:
        if_34_26_241_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_241_5_end:
    assert_241_5_end:
    addi t6, s0, 356
    li t5, 6
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    li t6, 73
    sw t6, 360(s0)
    cmp_247_12:
    addi t5, s0, 356
    li t4, 0
    addi t3, t4, 0
    slli t4, t3, 1
    add t4, t4, t3
    slli t4, t4, 2
    add t5, t5, t4
    lw t6, 4(t5)
    xori t6, t6, 73
    sltiu t6, t6, 1
    1:
    bool_end_247_12:
    assert_247_5:
        if_34_26_247_5:
        cmp_34_26_247_5:
        bne t6, zero, if_34_23_247_5_end
        1:
        if_34_26_247_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_247_5_end:
    assert_247_5_end:
    addi t6, s0, 356
    li t5, 1
    addi t4, t5, 0
    slli t5, t4, 1
    add t5, t5, t4
    slli t5, t5, 2
    add t6, t6, t5
    object_init_249_13:
        li t5, 74
        sw t5, 4(t6)
    object_init_249_13_end:
    cmp_250_12:
    addi t5, s0, 356
    li t4, 1
    addi t3, t4, 0
    slli t4, t3, 1
    add t4, t4, t3
    slli t4, t4, 2
    add t5, t5, t4
    lw t6, 4(t5)
    xori t6, t6, 74
    sltiu t6, t6, 1
    1:
    bool_end_250_12:
    assert_250_5:
        if_34_26_250_5:
        cmp_34_26_250_5:
        bne t6, zero, if_34_23_250_5_end
        1:
        if_34_26_250_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_250_5_end:
    assert_250_5_end:
    addi t6, s0, 380
    li t5, 64
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    addi t6, s0, 380
    li t5, 1
    slli t5, t5, ((5) & 31)
    add t6, t6, t5
    li t5, 1
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    li t5, 65518
    sw t5, 0(t6)
    cmp_254_12:
    addi t5, s0, 380
    li t4, 1
    slli t4, t4, ((5) & 31)
    add t5, t5, t4
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    li t4, 65518
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_254_12:
    assert_254_5:
        if_34_26_254_5:
        cmp_34_26_254_5:
        bne t6, zero, if_34_23_254_5_end
        1:
        if_34_26_254_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_254_5_end:
    assert_254_5_end:
    li t6, 8
    addi t3, s0, 380
    li t2, 1
    slli t2, t2, ((5) & 31)
    add t3, t3, t2
    addi t5, t3, 0
    addi t3, s0, 380
    li t2, 0
    slli t2, t2, ((5) & 31)
    add t3, t3, t2
    addi t4, t3, 0
    slli t6, t6, ((2) & 31)
    srli t2, t6, 2
    andi t6, t6, 3
    beqz t2, 2f
    1:
    lw t3, 0(t5)
    sw t3, 0(t4)
    addi t5, t5, 4
    addi t4, t4, 4
    addi t2, t2, -1
    bnez t2, 1b
    2:
    andi t3, t6, 2
    beqz t3, 3f
    lhu t3, 0(t5)
    sh t3, 0(t4)
    addi t5, t5, 2
    addi t4, t4, 2
    3:
    andi t6, t6, 1
    beqz t6, 4f
    lbu t3, 0(t5)
    sb t3, 0(t4)
    4:
    cmp_264_12:
    addi t5, s0, 380
    li t4, 0
    slli t4, t4, ((5) & 31)
    add t5, t5, t4
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    li t4, 65518
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_264_12:
    assert_264_5:
        if_34_26_264_5:
        cmp_34_26_264_5:
        bne t6, zero, if_34_23_264_5_end
        1:
        if_34_26_264_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_264_5_end:
    assert_264_5_end:
    cmp_265_12:
        li t4, 8
        addi t1, s0, 380
        li t0, 0
        slli t0, t0, ((5) & 31)
        add t1, t1, t0
        addi t3, t1, 0
        addi t1, s0, 380
        li t0, 1
        slli t0, t0, ((5) & 31)
        add t1, t1, t0
        addi t2, t1, 0
        slli t4, t4, ((2) & 31)
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltu t6, zero, t5
    1:
    bool_end_265_12:
    assert_265_5:
        if_34_26_265_5:
        cmp_34_26_265_5:
        bne t6, zero, if_34_23_265_5_end
        1:
        if_34_26_265_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_265_5_end:
    assert_265_5_end:
    li t6, -1
    sw t6, 636(s0)
    li t6, 2
    sw t6, 640(s0)
    cmp_272_12:
        li t5, 2
    xori t6, t5, 2
    sltiu t6, t6, 1
    1:
    bool_end_272_12:
    assert_272_5:
        if_34_26_272_5:
        cmp_34_26_272_5:
        bne t6, zero, if_34_23_272_5_end
        1:
        if_34_26_272_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    addi t5, s0, 636
    li t4, 0
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_273_12:
    assert_273_5:
        if_34_26_273_5:
        cmp_34_26_273_5:
        bne t6, zero, if_34_23_273_5_end
        1:
        if_34_26_273_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_273_5_end:
    assert_273_5_end:
    cmp_274_12:
    addi t5, s0, 636
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_274_12:
    assert_274_5:
        if_34_26_274_5:
        cmp_34_26_274_5:
        bne t6, zero, if_34_23_274_5_end
        1:
        if_34_26_274_5_code:
            li a0, 1
            li a7, 93
            ecall
        if_34_23_274_5_end:
    assert_274_5_end:
    sw zero, 644(s0)
    addi t6, s0, 648
    li t5, 32
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    print_278_5:
        li a0, 1
        addi a1, s0, 0
        li a2, 21
        li a7, 64
        ecall
    print_278_5_end:
    loop_279_5:
        lw t6, 644(s0)
        addi t6, t6, 1
        sw t6, 644(s0)
        addi t6, s0, 644
        sw t6, 776(s0)
        addi sp, sp, -128
        sw a0, 36(sp)
        sw a1, 40(sp)
        sw a2, 44(sp)
        sw a3, 48(sp)
        sw a4, 52(sp)
        sw a5, 56(sp)
        sw a6, 60(sp)
        sw a7, 64(sp)
        sw ra, 0(sp)
        sw gp, 8(sp)
        sw tp, 12(sp)
        sw s0, 28(sp)
        sw s1, 32(sp)
        sw s2, 68(sp)
        sw s3, 72(sp)
        sw s4, 76(sp)
        sw s5, 80(sp)
        sw s6, 84(sp)
        sw s7, 88(sp)
        sw s8, 92(sp)
        sw s9, 96(sp)
        sw s10, 100(sp)
        sw s11, 104(sp)
        sw t0, 16(sp)
        sw t1, 20(sp)
        sw t2, 24(sp)
        sw t3, 108(sp)
        sw t4, 112(sp)
        sw t5, 116(sp)
        sw t6, 120(sp)
        addi s1, s0, 776
        call print_num
        lw a0, 36(sp)
        lw a1, 40(sp)
        lw a2, 44(sp)
        lw a3, 48(sp)
        lw a4, 52(sp)
        lw a5, 56(sp)
        lw a6, 60(sp)
        lw a7, 64(sp)
        lw ra, 0(sp)
        lw gp, 8(sp)
        lw tp, 12(sp)
        lw s0, 28(sp)
        lw s1, 32(sp)
        lw s2, 68(sp)
        lw s3, 72(sp)
        lw s4, 76(sp)
        lw s5, 80(sp)
        lw s6, 84(sp)
        lw s7, 88(sp)
        lw s8, 92(sp)
        lw s9, 96(sp)
        lw s10, 100(sp)
        lw s11, 104(sp)
        lw t0, 16(sp)
        lw t1, 20(sp)
        lw t2, 24(sp)
        lw t3, 108(sp)
        lw t4, 112(sp)
        lw t5, 116(sp)
        lw t6, 120(sp)
        addi sp, sp, 128
        print_282_9:
            li a0, 1
            addi a1, s0, 61
            li a2, 2
            li a7, 64
            ecall
        print_282_9_end:
        print_283_9:
            li a0, 1
            addi a1, s0, 21
            li a2, 12
            li a7, 64
            ecall
        print_283_9_end:
        str_in_284_9:
            li a0, 0
            addi a1, s0, 649
            li a2, 127
            li a7, 63
            ecall
            sw a0, 776(s0)
            lw t6, 776(s0)
            sb t6, 648(s0)
            lb t6, 648(s0)
            addi t6, t6, -1
            sb t6, 648(s0)
        str_in_284_9_end:
        if_285_12:
        cmp_285_12:
        lb t6, 648(s0)
        bne t6, zero, if_287_19
        1:
        if_285_12_code:
            j loop_279_5_end
        if_287_19:
        cmp_287_19:
        lb t6, 648(s0)
        li t5, 4
        blt t5, t6, if_else_285_9
        1:
        if_287_19_code:
            print_288_13:
                li a0, 1
                addi a1, s0, 33
                li a2, 20
                li a7, 64
                ecall
            print_288_13_end:
            j loop_279_5
        if_else_285_9:
            print_291_13:
                li a0, 1
                addi a1, s0, 53
                li a2, 6
                li a7, 64
                ecall
            print_291_13_end:
            str_out_292_13:
                li a0, 1
                addi a1, s0, 649
                lb a2, 648(s0)
                li a7, 64
                ecall
            str_out_292_13_end:
            print_293_13:
                li a0, 1
                addi a1, s0, 59
                li a2, 1
                li a7, 64
                ecall
            print_293_13_end:
            print_294_13:
                li a0, 1
                addi a1, s0, 60
                li a2, 1
                li a7, 64
                ecall
            print_294_13_end:
        if_285_9_end:
    j loop_279_5
    loop_279_5_end:
    li a0, 0
    li a7, 93
    ecall
print_num:
    addi sp, sp, -16
    sw ra, 0(sp)
    addi t6, s1, 4
    li t5, 5
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    lw t6, 0(s1)
    lw t5, 0(t6)
    sw t5, 24(s1)
    sb zero, 28(s1)
    if_103_8:
    cmp_103_8:
    lw t6, 24(s1)
    bge t6, zero, if_103_5_end
    1:
    if_103_8_code:
        li t6, 1
        sb t6, 28(s1)
        lw t6, 24(s1)
        sw t6, 24(s1)
        lw t6, 24(s1)
        sub t6, zero, t6
        sw t6, 24(s1)
    if_103_5_end:
    li t6, 20
    sw t6, 29(s1)
    loop_109_5:
        lw t6, 29(s1)
        addi t6, t6, -1
        sw t6, 29(s1)
        li t6, 48
        lw t5, 24(s1)
        addi a0, t5, 0
        li a1, 10
        call .Lbaz_divide
        addi t5, a1, 0
        add t6, t6, t5
        sw t6, 33(s1)
        addi t6, s1, 4
        lw t5, 29(s1)
        add t6, t6, t5
        lw t5, 33(s1)
        sb t5, 0(t6)
        lw t6, 24(s1)
        sw t6, 24(s1)
        lw a0, 24(s1)
        li a1, 10
        call .Lbaz_divide
        sw a0, 24(s1)
        if_115_12:
        cmp_115_12:
        lw t6, 24(s1)
        bne t6, zero, if_115_9_end
        1:
        if_115_12_code:
            j loop_109_5_end
        if_115_9_end:
    j loop_109_5
    loop_109_5_end:
    if_118_8:
    cmp_118_8:
    lbu t6, 28(s1)
    beq t6, zero, if_118_5_end
    1:
    if_118_8_code:
        lw t6, 29(s1)
        addi t6, t6, -1
        sw t6, 29(s1)
        addi t6, s1, 4
        lw t5, 29(s1)
        add t6, t6, t5
        li t5, 45
        sb t5, 0(t6)
    if_118_5_end:
    sw zero, 33(s1)
    loop_124_5:
        addi t6, s1, 4
        lw t5, 33(s1)
        add t6, t6, t5
        addi t5, s1, 4
        lw t4, 29(s1)
        add t5, t5, t4
        lb t4, 0(t5)
        sb t4, 0(t6)
        lw t6, 33(s1)
        addi t6, t6, 1
        sw t6, 33(s1)
        lw t6, 29(s1)
        addi t6, t6, 1
        sw t6, 29(s1)
        if_128_12:
        cmp_128_12:
        lw t6, 29(s1)
        li t5, 20
        bne t6, t5, if_128_9_end
        1:
        if_128_12_code:
            j loop_124_5_end
        if_128_9_end:
    j loop_124_5
    loop_124_5_end:
    li a0, 1
    addi a1, s1, 4
    lw a2, 33(s1)
    li a7, 64
    ecall
    lw ra, 0(sp)
    addi sp, sp, 16
    ret
.equ print_num.size, 37
.Lbaz_divide:
    beqz a1, 5f
    srai t2, a0, 31
    srai t1, a1, 31
    xor a0, a0, t2
    sub a0, a0, t2
    xor a1, a1, t1
    sub a1, a1, t1
    xor t1, t1, t2
    li t0, 0
    li t3, 32
1:
    srli t4, a0, 31
    slli t0, t0, 1
    or t0, t0, t4
    slli a0, a0, 1
    bltu t0, a1, 2f
    sub t0, t0, a1
    ori a0, a0, 1
2:
    addi t3, t3, -1
    bnez t3, 1b
    xor a0, a0, t1
    sub a0, a0, t1
    xor a1, t0, t2
    sub a1, a1, t2
    ret
5:
    ebreak
    j 5b
.data
.balign 16
dat:
.ascii "hello world from baz\n"
.ascii "enter name:\n"
.ascii "that is not a name.\n"
.ascii "hello "
.ascii "."
.ascii "\n"
.ascii ": "
.rept 1
.word 1
.endr
.zero 12
.rept 1
.byte 3
.endr
.zero 127
dat.end:
.balign 16
vars:
.zero 65536
vars.end:
```

## With comments

```nasm

# [20:7] hello: i8 (0 B @ [s0])
# [21:5] prompt1: i8 (0 B @ [s0])
# [22:5] prompt2: i8 (0 B @ [s0])
# [23:5] prompt3: i8 (0 B @ [s0])
# [24:9] dot: i8 (0 B @ [s0])
# [25:10] nl: i8 (0 B @ [s0])
# [26:7] colon: i8 (0 B @ [s0])
# [27:8] nums: i32[4] (16 B @ [s0])
# [28:8] str1: str (128 B @ [s0 + 16])
# [34:13] x: bool (1 B @ [s0 + 145])
# [36:12] str: i8 (0 B @ [s0 + 145])
# [42:11] pt: point (8 B @ [s0 + 145])
# [49:10] arg: i32 (4 B @ [s0 + 145])
# [58:21] res: i32 (4 B @ [s0 + 145])
# [58:10] i: i32 (4 B @ [s0 + 149])
# [62:17] res: i32 (4 B @ [s0 + 145])
# [62:10] arg: i32 (4 B @ [s0 + 149])
# [68:10] arg: i32 (0 B @ [s0 + 145])
# [72:13] s: str (128 B @ [s0 + 145])
# [73:9] nbytes: i32 (4 B @ [s0 + 273])
# [77:14] s: str (128 B @ [s0 + 145])
# [81:21] res: point (8 B @ [s0 + 145])
# [86:22] res: object (12 B @ [s0 + 145])
# [90:7] const yes = 1
# [91:7] const no = 0
# [92:7] const maybe = -1
# [98:25] num: i32 (4 B @ [s0 + 145])
# [99:9] buf: i8[20] (20 B @ [s0 + 149])
# [100:9] n: i32 (4 B @ [s0 + 169])
# [101:9] is_negative: bool (1 B @ [s0 + 173])
# [108:9] i: i32 (4 B @ [s0 + 174])
# [111:13] ascii: i32 (4 B @ [s0 + 178])
# [123:9] write_pos: i32 (4 B @ [s0 + 178])
# [135:9] arr: i32[4] (16 B @ [s0 + 145])
# [138:9] answer: i32 (4 B @ [s0 + 161])
# [146:15] const maybe = 33
# [152:9] ix: i32 (4 B @ [s0 + 165])
# [164:9] arr1: i32[8] (32 B @ [s0 + 169])
# [179:9] arr3: i32 (0 B @ [s0 + 201])
# e: i32 (4 B @ [s0 + 201])
# i: i32 (4 B @ [s0 + 205])
# const n = 0
# [190:9] p: point (8 B @ [s0 + 201])
# [195:9] q: point (8 B @ [s0 + 209])
# [203:9] i: i32 (4 B @ [s0 + 217])
# [211:9] j: i32 (4 B @ [s0 + 221])
# [212:9] k: i32 (4 B @ [s0 + 225])
# [218:9] p0: point (8 B @ [s0 + 229])
# [221:9] pt: point (8 B @ [s0 + 237])
# [225:9] x: i32 (4 B @ [s0 + 245])
# [226:9] y: i32 (4 B @ [s0 + 249])
# [228:9] o1: object (12 B @ [s0 + 253])
# [233:9] p1: point (8 B @ [s0 + 265])
# [238:9] o2: object (12 B @ [s0 + 273])
# [243:9] o3: object[2] (24 B @ [s0 + 285])
# [252:9] worlds: world[8] (256 B @ [s0 + 309])
# [271:9] arr2: i32 (0 B @ [s0 + 565])
# [276:9] counter: i32 (4 B @ [s0 + 565])
# [277:9] nm: str (128 B @ [s0 + 569])
.option norvc
.option norelax
.text
.globl _start
_start:
    la s0, dat
# [1:1] # user types are defined using keyword `type`
# [3:1] # built-in types are `i63`, `i32`, `i16`, `i8` and `bool`
# [5:1] # default type is used if ommitted (`i64` on x86_64 and 'i32' on rv32i)
# [7:1] point : 8 B    fields:
# [7:1]       name :  offset :    size :  array? : array size
# [7:1]          x :       0 :       4 :      no :           
# [7:1]          y :       4 :       4 :      no :           
# 
# [9:1] object : 12 B    fields:
# [9:1]       name :  offset :    size :  array? : array size
# [9:1]        pos :       0 :       8 :      no :           
# [9:1]      color :       8 :       4 :      no :           
# 
# [11:1] world : 32 B    fields:
# [11:1]       name :  offset :    size :  array? : array size
# [11:1]  locations :       0 :      32 :     yes :          8
# 
# [13:1] str : 128 B    fields:
# [13:1]       name :  offset :    size :  array? : array size
# [13:1]        len :       0 :       1 :      no :           
# [13:1]       data :       1 :     127 :     yes :        127
# 
# [18:1] # initial data is initialized before variables
# [20:1] dat hello : i8[] = "hello world from baz\n"
# [20:7] hello: i8[21] (21 B @ [s0])
# [21:1] dat prompt1 : i8[] = "enter name:\n"
# [21:5] prompt1: i8[12] (12 B @ [s0 + 21])
# [22:1] dat prompt2 : i8[] = "that is not a name.\n"
# [22:5] prompt2: i8[20] (20 B @ [s0 + 33])
# [23:1] dat prompt3 : i8[] = "hello "
# [23:5] prompt3: i8[6] (6 B @ [s0 + 53])
# [24:1] dat dot : i8[] = "."
# [24:9] dot: i8[1] (1 B @ [s0 + 59])
# [25:1] dat nl : i8[] = "\n"
# [25:10] nl: i8[1] (1 B @ [s0 + 60])
# [26:1] dat colon : i8[] = ": "
# [26:7] colon: i8[2] (2 B @ [s0 + 61])
# [27:1] dat nums : [4] = { 1 }
# [27:8] nums: i32[4] (16 B @ [s0 + 63])
# [27:27] # remaining elements are zeroed
# [28:1] dat str1 : str = { 3 }
# [28:8] str1: str (128 B @ [s0 + 79])
# [28:27] # remaining fields are zeroed
# [30:1] # default is to inline functions
# [32:1] # single statement blocks can ommit { ... }
# [40:1] # function arguments and return are equivalent to mutable references
# [47:1] # default argument type is i64 on x86_64 and i32 on rv32i
# [54:1] # return target is specified as a variable, in this case `res`
# [56:1] # return variable is a mutable reference to destination
# [66:1] # array arguments are declared with type and []
# [90:7] const yes = 1
# [91:7] const no = 0
# [92:7] const maybe = -1
# [94:1] # constants can be declared in any scope and shadow outer declarations
# [96:1] # limited support for non-inlined functions
# 
main:
    # [135:5] var arr : i32[4]
    # [135:9] arr: i32[4] (16 B @ [s0 + 208])
    # [135:9] zero 4 * 4 B = 16 B
    sw zero, 208(s0)
    sw zero, 212(s0)
    sw zero, 216(s0)
    sw zero, 220(s0)
    # [136:5] # arrays are initialized to 0
    # [138:5] var answer
    # [138:9] answer: i32 (4 B @ [s0 + 224])
    # [138:9] zero 1 * 4 B = 4 B
    sw zero, 224(s0)
    # [139:5] assert(answer == 0)
    # [139:12] ? answer == 0
    # [139:12] ? answer == 0
    cmp_139_12:
    lw t6, 224(s0)
    sltiu t6, t6, 1
    1:
    bool_end_139_12:
    # [34:6] assert(x : bool)
    assert_139_5:
        # [139:5] alias x -> t6
        if_34_26_139_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_139_5:
        bne t6, zero, if_34_23_139_5_end
        1:
        if_34_26_139_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_139_5_end:
    assert_139_5_end:
    # [140:5] # variables without initializer are zeroed
    # [142:5] answer = maybe
    # [142:14] maybe
    li t6, -1
    sw t6, 224(s0)
    # [143:5] assert(answer == -1)
    # [143:12] ? answer == -1
    # [143:12] ? answer == -1
    cmp_143_12:
    lw t6, 224(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_143_12:
    # [34:6] assert(x : bool)
    assert_143_5:
        # [143:5] alias x -> t6
        if_34_26_143_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_143_5:
        bne t6, zero, if_34_23_143_5_end
        1:
        if_34_26_143_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_143_5_end:
    assert_143_5_end:
        # [146:15] const maybe = 33
        # [147:9] assert(maybe == 33)
        # [147:16] ? maybe == 33
        # [147:16] ? maybe == 33
        cmp_147_16:
        # [147:16] const eval to true
        bool_end_147_16:
        li t6, 1
        # [34:6] assert(x : bool)
        assert_147_9:
            # [147:9] alias x -> t6
            if_34_26_147_9:
            # [34:26] ? not x
            # [34:26] ? not x
            cmp_34_26_147_9:
            bne t6, zero, if_34_23_147_9_end
            1:
            if_34_26_147_9_code:
                # [34:32] exit(1)
                # [34:37] 1
                li a0, 1
                li a7, 93
                ecall
            if_34_23_147_9_end:
        assert_147_9_end:
    # [150:5] assert(maybe == -1)
    # [150:12] ? maybe == -1
    # [150:12] ? maybe == -1
    cmp_150_12:
    # [150:12] const eval to true
    bool_end_150_12:
    li t6, 1
    # [34:6] assert(x : bool)
    assert_150_5:
        # [150:5] alias x -> t6
        if_34_26_150_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_150_5:
        bne t6, zero, if_34_23_150_5_end
        1:
        if_34_26_150_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_150_5_end:
    assert_150_5_end:
    # [152:5] var ix = 1
    # [152:9] ix: i32 (4 B @ [s0 + 228])
    # [152:9] ix = 1
    # [152:14] 1
    li t6, 1
    sw t6, 228(s0)
    # [153:5] # variables can have an initial value that can be an expression
    # [155:5] arr[ix] = 2
    addi t6, s0, 208
    # [155:9] set array index
    # [155:9] ix
    lw t5, 228(s0)
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    # [155:15] 2
    li t5, 2
    sw t5, 0(t6)
    # [156:5] arr[ix + 1] = arr[ix]
    addi t6, s0, 208
    # [156:9] set array index
    # [156:9] ix
    lw t5, 228(s0)
    # [156:14] t5 + 1
    addi t5, t5, 1
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    # [156:19] arr[ix]
    addi t5, s0, 208
    # [156:23] set array index
    # [156:23] ix
    lw t4, 228(s0)
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t4, 0(t5)
    sw t4, 0(t6)
    # [157:5] assert(arr[1] == 2)
    # [157:12] ? arr[1] == 2
    # [157:12] ? arr[1] == 2
    cmp_157_12:
    addi t5, s0, 208
    # [157:16] set array index
    # [157:16] 1
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_157_12:
    # [34:6] assert(x : bool)
    assert_157_5:
        # [157:5] alias x -> t6
        if_34_26_157_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_157_5:
        bne t6, zero, if_34_23_157_5_end
        1:
        if_34_26_157_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_157_5_end:
    assert_157_5_end:
    # [158:5] assert(arr[2] == 2)
    # [158:12] ? arr[2] == 2
    # [158:12] ? arr[2] == 2
    cmp_158_12:
    addi t5, s0, 208
    # [158:16] set array index
    # [158:16] 2
    li t4, 2
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_158_12:
    # [34:6] assert(x : bool)
    assert_158_5:
        # [158:5] alias x -> t6
        if_34_26_158_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_158_5:
        bne t6, zero, if_34_23_158_5_end
        1:
        if_34_26_158_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_158_5_end:
    assert_158_5_end:
    # [160:5] array_copy(arr[2], arr, 2)
    # [160:29] 2
    # [160:29] 2
    li t6, 2
    # [160:16] arr[2]
    addi t3, s0, 208
    # [160:20] set array index
    # [160:20] 2
    li t2, 2
    slli t2, t2, ((2) & 31)
    add t3, t3, t2
    addi t5, t3, 0
    # [160:24] arr
    addi t4, s0, 208
    slli t6, t6, ((2) & 31)
    srli t2, t6, 2
    andi t6, t6, 3
    beqz t2, 2f
    1:
    lw t3, 0(t5)
    sw t3, 0(t4)
    addi t5, t5, 4
    addi t4, t4, 4
    addi t2, t2, -1
    bnez t2, 1b
    2:
    andi t3, t6, 2
    beqz t3, 3f
    lhu t3, 0(t5)
    sh t3, 0(t4)
    addi t5, t5, 2
    addi t4, t4, 2
    3:
    andi t6, t6, 1
    beqz t6, 4f
    lbu t3, 0(t5)
    sb t3, 0(t4)
    4:
    # [161:5] # copy from, to, number of elements
    # [162:5] assert(arr[0] == 2)
    # [162:12] ? arr[0] == 2
    # [162:12] ? arr[0] == 2
    cmp_162_12:
    addi t5, s0, 208
    # [162:16] set array index
    # [162:16] 0
    li t4, 0
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_162_12:
    # [34:6] assert(x : bool)
    assert_162_5:
        # [162:5] alias x -> t6
        if_34_26_162_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_162_5:
        bne t6, zero, if_34_23_162_5_end
        1:
        if_34_26_162_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_162_5_end:
    assert_162_5_end:
    # [164:5] var arr1 : i32[8]
    # [164:9] arr1: i32[8] (32 B @ [s0 + 232])
    # [164:9] zero 8 * 4 B = 32 B
    addi t6, s0, 232
    li t5, 8
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    # [165:5] array_copy(arr, arr1, 4)
    # [165:27] 4
    # [165:27] 4
    li t6, 4
    # [165:16] arr
    addi t5, s0, 208
    # [165:21] arr1
    addi t4, s0, 232
    slli t6, t6, ((2) & 31)
    srli t2, t6, 2
    andi t6, t6, 3
    beqz t2, 2f
    1:
    lw t3, 0(t5)
    sw t3, 0(t4)
    addi t5, t5, 4
    addi t4, t4, 4
    addi t2, t2, -1
    bnez t2, 1b
    2:
    andi t3, t6, 2
    beqz t3, 3f
    lhu t3, 0(t5)
    sh t3, 0(t4)
    addi t5, t5, 2
    addi t4, t4, 2
    3:
    andi t6, t6, 1
    beqz t6, 4f
    lbu t3, 0(t5)
    sb t3, 0(t4)
    4:
    # [166:5] assert(arrays_equal(arr, arr1, 4))
    # [166:12] ? arrays_equal(arr, arr1, 4)
    # [166:12] ? arrays_equal(arr, arr1, 4)
    cmp_166_12:
        # [166:12] t5 = arrays_equal(arr, arr1, 4)
        # [166:12] = expression
        # [166:12] arrays_equal(arr, arr1, 4)
        # [166:36] 4
        # [166:36] 4
        li t4, 4
        # [166:25] arr
        addi t3, s0, 208
        # [166:30] arr1
        addi t2, s0, 232
        slli t4, t4, ((2) & 31)
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltu t6, zero, t5
    1:
    bool_end_166_12:
    # [34:6] assert(x : bool)
    assert_166_5:
        # [166:5] alias x -> t6
        if_34_26_166_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_166_5:
        bne t6, zero, if_34_23_166_5_end
        1:
        if_34_26_166_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_166_5_end:
    assert_166_5_end:
    # [167:5] # `arrays_equal` is built-in function
    # [169:5] arr1[2] = -1
    addi t6, s0, 232
    # [169:10] set array index
    # [169:10] 2
    li t5, 2
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    # [169:16] -1
    li t5, -1
    sw t5, 0(t6)
    # [170:5] assert(not arrays_equal(arr, arr1, 4))
    # [170:12] ? not arrays_equal(arr, arr1, 4)
    # [170:12] ? not arrays_equal(arr, arr1, 4)
    cmp_170_12:
        # [170:16] t5 = arrays_equal(arr, arr1, 4)
        # [170:16] = expression
        # [170:16] arrays_equal(arr, arr1, 4)
        # [170:40] 4
        # [170:40] 4
        li t4, 4
        # [170:29] arr
        addi t3, s0, 208
        # [170:34] arr1
        addi t2, s0, 232
        slli t4, t4, ((2) & 31)
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltiu t6, t5, 1
    1:
    bool_end_170_12:
    # [34:6] assert(x : bool)
    assert_170_5:
        # [170:5] alias x -> t6
        if_34_26_170_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_170_5:
        bne t6, zero, if_34_23_170_5_end
        1:
        if_34_26_170_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_170_5_end:
    assert_170_5_end:
    # [172:5] ix = 3
    # [172:10] 3
    li t6, 3
    sw t6, 228(s0)
    # [173:5] arr[ix] = ~inv(arr[ix - 1])
    addi t6, s0, 208
    # [173:9] set array index
    # [173:9] ix
    lw t5, 228(s0)
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    # [173:16] arr = ~inv(arr[ix - 1])
    # [173:16] = expression
    # [173:16] ~inv(arr[ix - 1])
    addi t5, s0, 208
    # [173:24] set array index
    # [173:24] ix
    lw t4, 228(s0)
    # [173:29] t4 - 1
    addi t4, t4, -1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    # [58:6] inv(i : i32) : res i32
    inv_173_16:
        # [173:16] alias res -> arr
        # [173:16] alias i -> arr
        # [59:5] res = ~i
        # [59:12] ~i
        lw t4, 0(t5)
        sw t4, 0(t6)
        lw t4, 0(t6)
        xori t4, t4, -1
        sw t4, 0(t6)
    inv_173_16_end:
    lw t5, 0(t6)
    xori t5, t5, -1
    sw t5, 0(t6)
    # [174:5] assert(arr[ix] == 2)
    # [174:12] ? arr[ix] == 2
    # [174:12] ? arr[ix] == 2
    cmp_174_12:
    addi t5, s0, 208
    # [174:16] set array index
    # [174:16] ix
    lw t4, 228(s0)
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_174_12:
    # [34:6] assert(x : bool)
    assert_174_5:
        # [174:5] alias x -> t6
        if_34_26_174_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_174_5:
        bne t6, zero, if_34_23_174_5_end
        1:
        if_34_26_174_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_174_5_end:
    assert_174_5_end:
    # [176:5] faz(arr)
    # [68:6] faz(arg : i32[])
    faz_176_5:
        # [176:5] alias arg -> arr
        # [69:5] arg[1] = 0xfe
        addi t6, s0, 208
        # [69:9] set array index
        # [69:9] 1
        li t5, 1
        slli t5, t5, ((2) & 31)
        add t6, t6, t5
        # [69:14] 0xfe
        li t5, 254
        sw t5, 0(t6)
    faz_176_5_end:
    # [177:5] assert(arr[1] == 0xfe)
    # [177:12] ? arr[1] == 0xfe
    # [177:12] ? arr[1] == 0xfe
    cmp_177_12:
    addi t5, s0, 208
    # [177:16] set array index
    # [177:16] 1
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 254
    sltiu t6, t6, 1
    1:
    bool_end_177_12:
    # [34:6] assert(x : bool)
    assert_177_5:
        # [177:5] alias x -> t6
        if_34_26_177_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_177_5:
        bne t6, zero, if_34_23_177_5_end
        1:
        if_34_26_177_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_177_5_end:
    assert_177_5_end:
    # [179:5] var arr3 : [] = { 3, 5 }
    # [179:9] arr3: i32[2] (8 B @ [s0 + 264])
    # [179:9] arr3 = { 3, 5 }
    # [179:23] [0]
    # [179:23] 3
    li t6, 3
    sw t6, 264(s0)
    # [179:23] [1]
    # [179:26] 5
    li t6, 5
    sw t6, 268(s0)
    # [180:5] foo arr3
    # [180:9] e: i32 (t6)
    # [180:9] i: i32 (4 B @ [s0 + 276])
    # [180:9] const n = 2
    # [180:9] initiate iterator e
    addi t6, s0, 264
    # [180:9] initiate counter i
    sw zero, 276(s0)
    foo_180_5:
        # [181:9] e = e + i + n
        # [181:13] e
        lw t5, 0(t6)
        # [181:17] t5 + i
        lw t4, 276(s0)
        add t5, t5, t4
        # [181:21] t5 + n
        addi t5, t5, 2
        sw t5, 0(t6)
        foo_180_5_continue:
            addi t6, t6, 4
            lw t5, 276(s0)
            addi t5, t5, 1
            sw t5, 276(s0)
            li t4, 2
            bne t5, t4, foo_180_5
            1:
    foo_180_5_end:
    # [183:5] assert(arr3[0] == 3 + 0 + 2)
    # [183:12] ? arr3[0] == 3 + 0 + 2
    # [183:12] ? arr3[0] == 3 + 0 + 2
    cmp_183_12:
    addi t5, s0, 264
    # [183:17] set array index
    # [183:17] 0
    li t4, 0
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
        # [183:23] 3
        li t4, 3
        # [183:27] t4 + 0
        # [183:31] t4 + 2
        addi t4, t4, 2
    lw t6, 0(t5)
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_183_12:
    # [34:6] assert(x : bool)
    assert_183_5:
        # [183:5] alias x -> t6
        if_34_26_183_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_183_5:
        bne t6, zero, if_34_23_183_5_end
        1:
        if_34_26_183_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_183_5_end:
    assert_183_5_end:
    # [184:5] assert(arr3[1] == 5 + 1 + 2)
    # [184:12] ? arr3[1] == 5 + 1 + 2
    # [184:12] ? arr3[1] == 5 + 1 + 2
    cmp_184_12:
    addi t5, s0, 264
    # [184:17] set array index
    # [184:17] 1
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
        # [184:23] 5
        li t4, 5
        # [184:27] t4 + 1
        addi t4, t4, 1
        # [184:31] t4 + 2
        addi t4, t4, 2
    lw t6, 0(t5)
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_184_12:
    # [34:6] assert(x : bool)
    assert_184_5:
        # [184:5] alias x -> t6
        if_34_26_184_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_184_5:
        bne t6, zero, if_34_23_184_5_end
        1:
        if_34_26_184_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_184_5_end:
    assert_184_5_end:
    # [185:5] # `foo` is a language construct that iterates over an array injecting:
    # [186:5] # `e`: current element
    # [187:5] # `i`: index starting at 0
    # [188:5] # `n`: constant array size
    # [190:5] var p : point = {0, 0}
    # [190:9] p: point (8 B @ [s0 + 272])
    # [190:9] p = {0, 0}
    # [190:22] copy field 'x'
    sw zero, 272(s0)
    # [190:25] copy field 'y'
    sw zero, 276(s0)
    # [191:5] fooz(p)
    # [42:6] fooz(pt : point)
    fooz_191_5:
        # [191:5] alias pt -> p
        # [43:5] pt.x = 0b10
        # [43:12] 0b10
        li t6, 2
        sw t6, 272(s0)
        # [43:20] # binary value 2
        # [44:5] pt.y = 0xb
        # [44:12] 0xb
        li t6, 11
        sw t6, 276(s0)
        # [44:20] # hex value 11
    fooz_191_5_end:
    # [192:5] assert(p.x == 2)
    # [192:12] ? p.x == 2
    # [192:12] ? p.x == 2
    cmp_192_12:
    lw t6, 272(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_192_12:
    # [34:6] assert(x : bool)
    assert_192_5:
        # [192:5] alias x -> t6
        if_34_26_192_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_192_5:
        bne t6, zero, if_34_23_192_5_end
        1:
        if_34_26_192_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_192_5_end:
    assert_192_5_end:
    # [193:5] assert(p.y == 0xb)
    # [193:12] ? p.y == 0xb
    # [193:12] ? p.y == 0xb
    cmp_193_12:
    lw t6, 276(s0)
    xori t6, t6, 11
    sltiu t6, t6, 1
    1:
    bool_end_193_12:
    # [34:6] assert(x : bool)
    assert_193_5:
        # [193:5] alias x -> t6
        if_34_26_193_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_193_5:
        bne t6, zero, if_34_23_193_5_end
        1:
        if_34_26_193_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_193_5_end:
    assert_193_5_end:
    # [195:5] var q : point = p
    # [195:9] q: point (8 B @ [s0 + 280])
    # [195:9] q = p
    lw t6, 272(s0)
    sw t6, 280(s0)
    lw t6, 276(s0)
    sw t6, 284(s0)
    # [196:5] assert(equal(p, q))
    # [196:12] ? equal(p, q)
    # [196:12] ? equal(p, q)
    cmp_196_12:
        # [196:12] t5 = equal(p, q)
        # [196:12] = expression
        # [196:12] equal(p, q)
        # [196:18] p
        addi t3, s0, 272
        # [196:21] q
        addi t2, s0, 280
        li t4, 8
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltu t6, zero, t5
    1:
    bool_end_196_12:
    # [34:6] assert(x : bool)
    assert_196_5:
        # [196:5] alias x -> t6
        if_34_26_196_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_196_5:
        bne t6, zero, if_34_23_196_5_end
        1:
        if_34_26_196_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_196_5_end:
    assert_196_5_end:
    # [197:5] # `equal` is built-in function to compare user types for equality or same
    # [198:5] # size arrays
    # [200:5] q.x = 3
    # [200:11] 3
    li t6, 3
    sw t6, 280(s0)
    # [201:5] assert(not equal(p, q))
    # [201:12] ? not equal(p, q)
    # [201:12] ? not equal(p, q)
    cmp_201_12:
        # [201:16] t5 = equal(p, q)
        # [201:16] = expression
        # [201:16] equal(p, q)
        # [201:22] p
        addi t3, s0, 272
        # [201:25] q
        addi t2, s0, 280
        li t4, 8
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltiu t6, t5, 1
    1:
    bool_end_201_12:
    # [34:6] assert(x : bool)
    assert_201_5:
        # [201:5] alias x -> t6
        if_34_26_201_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_201_5:
        bne t6, zero, if_34_23_201_5_end
        1:
        if_34_26_201_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_201_5_end:
    assert_201_5_end:
    # [203:5] var i = 0
    # [203:9] i: i32 (4 B @ [s0 + 288])
    # [203:9] i = 0
    # [203:13] 0
    sw zero, 288(s0)
    # [204:5] bar(i)
    # [49:6] bar(arg)
    bar_204_5:
        # [204:5] alias arg -> i
        if_50_8_204_5:
        # [50:8] ? arg == 0
        # [50:8] ? arg == 0
        cmp_50_8_204_5:
        lw t6, 288(s0)
        bne t6, zero, if_50_5_204_5_end
        1:
        if_50_8_204_5_code:
            # [50:17] return
            j bar_204_5_end
        if_50_5_204_5_end:
        # [51:5] arg = 0xff
        # [51:11] 0xff
        li t6, 255
        sw t6, 288(s0)
    bar_204_5_end:
    # [205:5] assert(i == 0)
    # [205:12] ? i == 0
    # [205:12] ? i == 0
    cmp_205_12:
    lw t6, 288(s0)
    sltiu t6, t6, 1
    1:
    bool_end_205_12:
    # [34:6] assert(x : bool)
    assert_205_5:
        # [205:5] alias x -> t6
        if_34_26_205_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_205_5:
        bne t6, zero, if_34_23_205_5_end
        1:
        if_34_26_205_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_205_5_end:
    assert_205_5_end:
    # [207:5] i = 1
    # [207:9] 1
    li t6, 1
    sw t6, 288(s0)
    # [208:5] bar(i)
    # [49:6] bar(arg)
    bar_208_5:
        # [208:5] alias arg -> i
        if_50_8_208_5:
        # [50:8] ? arg == 0
        # [50:8] ? arg == 0
        cmp_50_8_208_5:
        lw t6, 288(s0)
        bne t6, zero, if_50_5_208_5_end
        1:
        if_50_8_208_5_code:
            # [50:17] return
            j bar_208_5_end
        if_50_5_208_5_end:
        # [51:5] arg = 0xff
        # [51:11] 0xff
        li t6, 255
        sw t6, 288(s0)
    bar_208_5_end:
    # [209:5] assert(i == 0xff)
    # [209:12] ? i == 0xff
    # [209:12] ? i == 0xff
    cmp_209_12:
    lw t6, 288(s0)
    xori t6, t6, 255
    sltiu t6, t6, 1
    1:
    bool_end_209_12:
    # [34:6] assert(x : bool)
    assert_209_5:
        # [209:5] alias x -> t6
        if_34_26_209_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_209_5:
        bne t6, zero, if_34_23_209_5_end
        1:
        if_34_26_209_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_209_5_end:
    assert_209_5_end:
    # [211:5] var j = 1
    # [211:9] j: i32 (4 B @ [s0 + 292])
    # [211:9] j = 1
    # [211:13] 1
    li t6, 1
    sw t6, 292(s0)
    # [212:5] var k = baz(j)
    # [212:9] k: i32 (4 B @ [s0 + 296])
    # [212:9] k = baz(j)
    # [212:13] k = baz(j)
    # [212:13] = expression
    # [212:13] baz(j)
    # [62:6] baz(arg) : res
    baz_212_13:
        # [212:13] alias res -> k
        # [212:13] alias arg -> j
        # [63:5] res = arg * 2
        # [63:11] arg
        lw t6, 292(s0)
        # [63:17] t6 * 2
        # [63:17] dst is reg, src is const
        slli t6, t6, ((1) & 31)
        sw t6, 296(s0)
    baz_212_13_end:
    # [213:5] assert(k == 2)
    # [213:12] ? k == 2
    # [213:12] ? k == 2
    cmp_213_12:
    lw t6, 296(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_213_12:
    # [34:6] assert(x : bool)
    assert_213_5:
        # [213:5] alias x -> t6
        if_34_26_213_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_213_5:
        bne t6, zero, if_34_23_213_5_end
        1:
        if_34_26_213_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_213_5_end:
    assert_213_5_end:
    # [215:5] k = baz(1)
    # [215:9] k = baz(1)
    # [215:9] = expression
    # [215:9] baz(1)
    # [62:6] baz(arg) : res
    baz_215_9:
        # [215:9] alias res -> k
        # [215:9] alias arg -> 1
        # [63:5] res = arg * 2
        # [63:11] arg
        li t6, 1
        # [63:17] t6 * 2
        # [63:17] dst is reg, src is const
        slli t6, t6, ((1) & 31)
        sw t6, 296(s0)
    baz_215_9_end:
    # [216:5] assert(k == 2)
    # [216:12] ? k == 2
    # [216:12] ? k == 2
    cmp_216_12:
    lw t6, 296(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_216_12:
    # [34:6] assert(x : bool)
    assert_216_5:
        # [216:5] alias x -> t6
        if_34_26_216_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_216_5:
        bne t6, zero, if_34_23_216_5_end
        1:
        if_34_26_216_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_216_5_end:
    assert_216_5_end:
    # [218:5] var p0 : point = {baz(3), 0}
    # [218:9] p0: point (8 B @ [s0 + 300])
    # [218:9] p0 = {baz(3), 0}
    # [218:23] copy field 'x'
    # [218:23] p0.x = baz(3)
    # [218:23] = expression
    # [218:23] baz(3)
    # [62:6] baz(arg) : res
    baz_218_23:
        # [218:23] alias res -> p0.x
        # [218:23] alias arg -> 3
        # [63:5] res = arg * 2
        # [63:11] arg
        li t6, 3
        # [63:17] t6 * 2
        # [63:17] dst is reg, src is const
        slli t6, t6, ((1) & 31)
        sw t6, 300(s0)
    baz_218_23_end:
    # [218:31] copy field 'y'
    sw zero, 304(s0)
    # [219:5] assert(p0.x == 6)
    # [219:12] ? p0.x == 6
    # [219:12] ? p0.x == 6
    cmp_219_12:
    lw t6, 300(s0)
    xori t6, t6, 6
    sltiu t6, t6, 1
    1:
    bool_end_219_12:
    # [34:6] assert(x : bool)
    assert_219_5:
        # [219:5] alias x -> t6
        if_34_26_219_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_219_5:
        bne t6, zero, if_34_23_219_5_end
        1:
        if_34_26_219_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_219_5_end:
    assert_219_5_end:
    # [221:5] var pt : point = point_init()
    # [221:9] pt: point (8 B @ [s0 + 308])
    # [221:9] pt = point_init()
    # [221:22] point_init()
    # [81:6] point_init() : res point
    point_init_221_22:
        # [221:22] alias res -> pt
        # [82:5] res.x = -1
        # [82:14] -1
        li t6, -1
        sw t6, 308(s0)
        # [83:5] res.y = -2
        # [83:14] -2
        li t6, -2
        sw t6, 312(s0)
    point_init_221_22_end:
    # [222:5] assert(pt.x == -1)
    # [222:12] ? pt.x == -1
    # [222:12] ? pt.x == -1
    cmp_222_12:
    lw t6, 308(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_222_12:
    # [34:6] assert(x : bool)
    assert_222_5:
        # [222:5] alias x -> t6
        if_34_26_222_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_222_5:
        bne t6, zero, if_34_23_222_5_end
        1:
        if_34_26_222_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_222_5_end:
    assert_222_5_end:
    # [223:5] assert(pt.y == -2)
    # [223:12] ? pt.y == -2
    # [223:12] ? pt.y == -2
    cmp_223_12:
    lw t6, 312(s0)
    xori t6, t6, -2
    sltiu t6, t6, 1
    1:
    bool_end_223_12:
    # [34:6] assert(x : bool)
    assert_223_5:
        # [223:5] alias x -> t6
        if_34_26_223_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_223_5:
        bne t6, zero, if_34_23_223_5_end
        1:
        if_34_26_223_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_223_5_end:
    assert_223_5_end:
    # [225:5] var x = 1
    # [225:9] x: i32 (4 B @ [s0 + 316])
    # [225:9] x = 1
    # [225:13] 1
    li t6, 1
    sw t6, 316(s0)
    # [226:5] var y = 2
    # [226:9] y: i32 (4 B @ [s0 + 320])
    # [226:9] y = 2
    # [226:13] 2
    li t6, 2
    sw t6, 320(s0)
    # [228:5] var o1 : object = {{x * 10, y}, 0xff0000}
    # [228:9] o1: object (12 B @ [s0 + 324])
    # [228:9] o1 = {{x * 10, y}, 0xff0000}
    # [228:24] copy field 'pos'
    # [228:25] copy field 'x'
    # [228:25] x
    lw t6, 316(s0)
    # [228:29] t6 * 10
    # [228:29] dst is reg, src is const
    addi t5, t6, 0
    slli t6, t5, 2
    add t6, t6, t5
    slli t6, t6, 1
    sw t6, 324(s0)
    # [228:33] copy field 'y'
    lw t6, 320(s0)
    sw t6, 328(s0)
    # [228:37] copy field 'color'
    li t6, 16711680
    sw t6, 332(s0)
    # [229:5] assert(o1.pos.x == 10)
    # [229:12] ? o1.pos.x == 10
    # [229:12] ? o1.pos.x == 10
    cmp_229_12:
    lw t6, 324(s0)
    xori t6, t6, 10
    sltiu t6, t6, 1
    1:
    bool_end_229_12:
    # [34:6] assert(x : bool)
    assert_229_5:
        # [229:5] alias x -> t6
        if_34_26_229_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_229_5:
        bne t6, zero, if_34_23_229_5_end
        1:
        if_34_26_229_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_229_5_end:
    assert_229_5_end:
    # [230:5] assert(o1.pos.y == 2)
    # [230:12] ? o1.pos.y == 2
    # [230:12] ? o1.pos.y == 2
    cmp_230_12:
    lw t6, 328(s0)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_230_12:
    # [34:6] assert(x : bool)
    assert_230_5:
        # [230:5] alias x -> t6
        if_34_26_230_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_230_5:
        bne t6, zero, if_34_23_230_5_end
        1:
        if_34_26_230_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_230_5_end:
    assert_230_5_end:
    # [231:5] assert(o1.color == 0xff0000)
    # [231:12] ? o1.color == 0xff0000
    # [231:12] ? o1.color == 0xff0000
    cmp_231_12:
    lw t6, 332(s0)
    li t5, 16711680
    xor t6, t6, t5
    sltiu t6, t6, 1
    1:
    bool_end_231_12:
    # [34:6] assert(x : bool)
    assert_231_5:
        # [231:5] alias x -> t6
        if_34_26_231_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_231_5:
        bne t6, zero, if_34_23_231_5_end
        1:
        if_34_26_231_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_231_5_end:
    assert_231_5_end:
    # [233:5] var p1 : point = {-x, -y}
    # [233:9] p1: point (8 B @ [s0 + 336])
    # [233:9] p1 = {-x, -y}
    # [233:23] copy field 'x'
    lw t6, 316(s0)
    sw t6, 336(s0)
    lw t6, 336(s0)
    sub t6, zero, t6
    sw t6, 336(s0)
    # [233:27] copy field 'y'
    lw t6, 320(s0)
    sw t6, 340(s0)
    lw t6, 340(s0)
    sub t6, zero, t6
    sw t6, 340(s0)
    # [234:5] o1.pos = p1
    lw t6, 336(s0)
    sw t6, 324(s0)
    lw t6, 340(s0)
    sw t6, 328(s0)
    # [235:5] assert(o1.pos.x == -1)
    # [235:12] ? o1.pos.x == -1
    # [235:12] ? o1.pos.x == -1
    cmp_235_12:
    lw t6, 324(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_235_12:
    # [34:6] assert(x : bool)
    assert_235_5:
        # [235:5] alias x -> t6
        if_34_26_235_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_235_5:
        bne t6, zero, if_34_23_235_5_end
        1:
        if_34_26_235_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_235_5_end:
    assert_235_5_end:
    # [236:5] assert(o1.pos.y == -2)
    # [236:12] ? o1.pos.y == -2
    # [236:12] ? o1.pos.y == -2
    cmp_236_12:
    lw t6, 328(s0)
    xori t6, t6, -2
    sltiu t6, t6, 1
    1:
    bool_end_236_12:
    # [34:6] assert(x : bool)
    assert_236_5:
        # [236:5] alias x -> t6
        if_34_26_236_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_236_5:
        bne t6, zero, if_34_23_236_5_end
        1:
        if_34_26_236_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_236_5_end:
    assert_236_5_end:
    # [238:5] var o2 : object = o1
    # [238:9] o2: object (12 B @ [s0 + 344])
    # [238:9] o2 = o1
    lw t6, 324(s0)
    sw t6, 344(s0)
    lw t6, 328(s0)
    sw t6, 348(s0)
    lw t6, 332(s0)
    sw t6, 352(s0)
    # [239:5] assert(o2.pos.x == -1)
    # [239:12] ? o2.pos.x == -1
    # [239:12] ? o2.pos.x == -1
    cmp_239_12:
    lw t6, 344(s0)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_239_12:
    # [34:6] assert(x : bool)
    assert_239_5:
        # [239:5] alias x -> t6
        if_34_26_239_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_239_5:
        bne t6, zero, if_34_23_239_5_end
        1:
        if_34_26_239_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_239_5_end:
    assert_239_5_end:
    # [240:5] assert(o2.pos.y == -2)
    # [240:12] ? o2.pos.y == -2
    # [240:12] ? o2.pos.y == -2
    cmp_240_12:
    lw t6, 348(s0)
    xori t6, t6, -2
    sltiu t6, t6, 1
    1:
    bool_end_240_12:
    # [34:6] assert(x : bool)
    assert_240_5:
        # [240:5] alias x -> t6
        if_34_26_240_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_240_5:
        bne t6, zero, if_34_23_240_5_end
        1:
        if_34_26_240_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_240_5_end:
    assert_240_5_end:
    # [241:5] assert(o2.color == 0xff0000)
    # [241:12] ? o2.color == 0xff0000
    # [241:12] ? o2.color == 0xff0000
    cmp_241_12:
    lw t6, 352(s0)
    li t5, 16711680
    xor t6, t6, t5
    sltiu t6, t6, 1
    1:
    bool_end_241_12:
    # [34:6] assert(x : bool)
    assert_241_5:
        # [241:5] alias x -> t6
        if_34_26_241_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_241_5:
        bne t6, zero, if_34_23_241_5_end
        1:
        if_34_26_241_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_241_5_end:
    assert_241_5_end:
    # [243:5] var o3 : object[2]
    # [243:9] o3: object[2] (24 B @ [s0 + 356])
    # [243:9] zero 2 * 12 B = 24 B
    addi t6, s0, 356
    li t5, 6
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    # [244:5] o3.pos.y = 73
    # [244:16] 73
    li t6, 73
    sw t6, 360(s0)
    # [245:5] # index 0 in an array can be accessed without array index
    # [247:5] assert(o3[0].pos.y == 73)
    # [247:12] ? o3[0].pos.y == 73
    # [247:12] ? o3[0].pos.y == 73
    cmp_247_12:
    addi t5, s0, 356
    # [247:15] set array index
    # [247:15] 0
    li t4, 0
    addi t3, t4, 0
    slli t4, t3, 1
    add t4, t4, t3
    slli t4, t4, 2
    add t5, t5, t4
    lw t6, 4(t5)
    xori t6, t6, 73
    sltiu t6, t6, 1
    1:
    bool_end_247_12:
    # [34:6] assert(x : bool)
    assert_247_5:
        # [247:5] alias x -> t6
        if_34_26_247_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_247_5:
        bne t6, zero, if_34_23_247_5_end
        1:
        if_34_26_247_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_247_5_end:
    assert_247_5_end:
    # [249:5] o3[1] = object_init()
    addi t6, s0, 356
    # [249:8] set array index
    # [249:8] 1
    li t5, 1
    addi t4, t5, 0
    slli t5, t4, 1
    add t5, t5, t4
    slli t5, t5, 2
    add t6, t6, t5
    # [249:13] object_init()
    # [86:6] object_init() : res object
    object_init_249_13:
        # [249:13] alias res -> o3
        # [87:5] res.pos.y = 74
        # [87:17] 74
        li t5, 74
        sw t5, 4(t6)
    object_init_249_13_end:
    # [250:5] assert(o3[1].pos.y == 74)
    # [250:12] ? o3[1].pos.y == 74
    # [250:12] ? o3[1].pos.y == 74
    cmp_250_12:
    addi t5, s0, 356
    # [250:15] set array index
    # [250:15] 1
    li t4, 1
    addi t3, t4, 0
    slli t4, t3, 1
    add t4, t4, t3
    slli t4, t4, 2
    add t5, t5, t4
    lw t6, 4(t5)
    xori t6, t6, 74
    sltiu t6, t6, 1
    1:
    bool_end_250_12:
    # [34:6] assert(x : bool)
    assert_250_5:
        # [250:5] alias x -> t6
        if_34_26_250_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_250_5:
        bne t6, zero, if_34_23_250_5_end
        1:
        if_34_26_250_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_250_5_end:
    assert_250_5_end:
    # [252:5] var worlds : world[8]
    # [252:9] worlds: world[8] (256 B @ [s0 + 380])
    # [252:9] zero 8 * 32 B = 256 B
    addi t6, s0, 380
    li t5, 64
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    # [253:5] worlds[1].locations[1] = 0xffee
    addi t6, s0, 380
    # [253:12] set array index
    # [253:12] 1
    li t5, 1
    slli t5, t5, ((5) & 31)
    add t6, t6, t5
    # [253:25] set array index
    # [253:25] 1
    li t5, 1
    slli t5, t5, ((2) & 31)
    add t6, t6, t5
    # [253:30] 0xffee
    li t5, 65518
    sw t5, 0(t6)
    # [254:5] assert(worlds[1].locations[1] == 0xffee)
    # [254:12] ? worlds[1].locations[1] == 0xffee
    # [254:12] ? worlds[1].locations[1] == 0xffee
    cmp_254_12:
    addi t5, s0, 380
    # [254:19] set array index
    # [254:19] 1
    li t4, 1
    slli t4, t4, ((5) & 31)
    add t5, t5, t4
    # [254:32] set array index
    # [254:32] 1
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    li t4, 65518
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_254_12:
    # [34:6] assert(x : bool)
    assert_254_5:
        # [254:5] alias x -> t6
        if_34_26_254_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_254_5:
        bne t6, zero, if_34_23_254_5_end
        1:
        if_34_26_254_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_254_5_end:
    assert_254_5_end:
    # [256:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
    # [259:9] array_size_of(worlds.locations)
    # [259:9] t6 = array_size_of(worlds.locations)
    # [259:9] = expression
    # [259:9] array_size_of(worlds.locations)
    li t6, 8
    # [257:9] worlds[1].locations
    addi t3, s0, 380
    # [257:16] set array index
    # [257:16] 1
    li t2, 1
    slli t2, t2, ((5) & 31)
    add t3, t3, t2
    addi t5, t3, 0
    # [258:9] worlds[0].locations
    addi t3, s0, 380
    # [258:16] set array index
    # [258:16] 0
    li t2, 0
    slli t2, t2, ((5) & 31)
    add t3, t3, t2
    addi t4, t3, 0
    slli t6, t6, ((2) & 31)
    srli t2, t6, 2
    andi t6, t6, 3
    beqz t2, 2f
    1:
    lw t3, 0(t5)
    sw t3, 0(t4)
    addi t5, t5, 4
    addi t4, t4, 4
    addi t2, t2, -1
    bnez t2, 1b
    2:
    andi t3, t6, 2
    beqz t3, 3f
    lhu t3, 0(t5)
    sh t3, 0(t4)
    addi t5, t5, 2
    addi t4, t4, 2
    3:
    andi t6, t6, 1
    beqz t6, 4f
    lbu t3, 0(t5)
    sb t3, 0(t4)
    4:
    # [261:5] # `array_copy` is built-in and can use indexed positions
    # [262:5] # `array_size_of` is built-in
    # [264:5] assert(worlds[0].locations[1] == 0xffee)
    # [264:12] ? worlds[0].locations[1] == 0xffee
    # [264:12] ? worlds[0].locations[1] == 0xffee
    cmp_264_12:
    addi t5, s0, 380
    # [264:19] set array index
    # [264:19] 0
    li t4, 0
    slli t4, t4, ((5) & 31)
    add t5, t5, t4
    # [264:32] set array index
    # [264:32] 1
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    li t4, 65518
    xor t6, t6, t4
    sltiu t6, t6, 1
    1:
    bool_end_264_12:
    # [34:6] assert(x : bool)
    assert_264_5:
        # [264:5] alias x -> t6
        if_34_26_264_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_264_5:
        bne t6, zero, if_34_23_264_5_end
        1:
        if_34_26_264_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_264_5_end:
    assert_264_5_end:
    # [265:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
    # [265:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    # [265:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_265_12:
        # [265:12] t5 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
        # [265:12] = expression
        # [265:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
        # [268:14] array_size_of(worlds.locations)
        # [268:14] t4 = array_size_of(worlds.locations)
        # [268:14] = expression
        # [268:14] array_size_of(worlds.locations)
        li t4, 8
        # [266:14] worlds[0].locations
        addi t1, s0, 380
        # [266:21] set array index
        # [266:21] 0
        li t0, 0
        slli t0, t0, ((5) & 31)
        add t1, t1, t0
        addi t3, t1, 0
        # [267:14] worlds[1].locations
        addi t1, s0, 380
        # [267:21] set array index
        # [267:21] 1
        li t0, 1
        slli t0, t0, ((5) & 31)
        add t1, t1, t0
        addi t2, t1, 0
        slli t4, t4, ((2) & 31)
        srli t1, t4, 2
        andi t4, t4, 3
        beqz t1, 2f
        1:
        lw t5, 0(t3)
        lw t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 4
        addi t2, t2, 4
        addi t1, t1, -1
        bnez t1, 1b
        2:
        andi t5, t4, 2
        beqz t5, 3f
        lhu t5, 0(t3)
        lhu t0, 0(t2)
        bne t5, t0, 5f
        addi t3, t3, 2
        addi t2, t2, 2
        3:
        andi t4, t4, 1
        beqz t4, 4f
        lbu t5, 0(t3)
        lbu t0, 0(t2)
        bne t5, t0, 5f
        4:
        li t5, 1
        j 6f
        5:
        li t5, 0
        6:
    sltu t6, zero, t5
    1:
    bool_end_265_12:
    # [34:6] assert(x : bool)
    assert_265_5:
        # [265:5] alias x -> t6
        if_34_26_265_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_265_5:
        bne t6, zero, if_34_23_265_5_end
        1:
        if_34_26_265_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_265_5_end:
    assert_265_5_end:
    # [271:5] var arr2 : [] = { -1, 2 }
    # [271:9] arr2: i32[2] (8 B @ [s0 + 636])
    # [271:9] arr2 = { -1, 2 }
    # [271:23] [0]
    # [271:24] -1
    li t6, -1
    sw t6, 636(s0)
    # [271:23] [1]
    # [271:27] 2
    li t6, 2
    sw t6, 640(s0)
    # [272:5] assert(array_size_of(arr2) == 2)
    # [272:12] ? array_size_of(arr2) == 2
    # [272:12] ? array_size_of(arr2) == 2
    cmp_272_12:
        # [272:12] t5 = array_size_of(arr2)
        # [272:12] = expression
        # [272:12] array_size_of(arr2)
        li t5, 2
    xori t6, t5, 2
    sltiu t6, t6, 1
    1:
    bool_end_272_12:
    # [34:6] assert(x : bool)
    assert_272_5:
        # [272:5] alias x -> t6
        if_34_26_272_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_272_5:
        bne t6, zero, if_34_23_272_5_end
        1:
        if_34_26_272_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_272_5_end:
    assert_272_5_end:
    # [273:5] assert(arr2[0] == -1)
    # [273:12] ? arr2[0] == -1
    # [273:12] ? arr2[0] == -1
    cmp_273_12:
    addi t5, s0, 636
    # [273:17] set array index
    # [273:17] 0
    li t4, 0
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, -1
    sltiu t6, t6, 1
    1:
    bool_end_273_12:
    # [34:6] assert(x : bool)
    assert_273_5:
        # [273:5] alias x -> t6
        if_34_26_273_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_273_5:
        bne t6, zero, if_34_23_273_5_end
        1:
        if_34_26_273_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_273_5_end:
    assert_273_5_end:
    # [274:5] assert(arr2[1] == 2)
    # [274:12] ? arr2[1] == 2
    # [274:12] ? arr2[1] == 2
    cmp_274_12:
    addi t5, s0, 636
    # [274:17] set array index
    # [274:17] 1
    li t4, 1
    slli t4, t4, ((2) & 31)
    add t5, t5, t4
    lw t6, 0(t5)
    xori t6, t6, 2
    sltiu t6, t6, 1
    1:
    bool_end_274_12:
    # [34:6] assert(x : bool)
    assert_274_5:
        # [274:5] alias x -> t6
        if_34_26_274_5:
        # [34:26] ? not x
        # [34:26] ? not x
        cmp_34_26_274_5:
        bne t6, zero, if_34_23_274_5_end
        1:
        if_34_26_274_5_code:
            # [34:32] exit(1)
            # [34:37] 1
            li a0, 1
            li a7, 93
            ecall
        if_34_23_274_5_end:
    assert_274_5_end:
    # [276:5] var counter
    # [276:9] counter: i32 (4 B @ [s0 + 644])
    # [276:9] zero 1 * 4 B = 4 B
    sw zero, 644(s0)
    # [277:5] var nm : str
    # [277:9] nm: str (128 B @ [s0 + 648])
    # [277:9] zero 1 * 128 B = 128 B
    addi t6, s0, 648
    li t5, 32
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    # [278:5] print(hello)
    # [36:6] print(str : i8[])
    print_278_5:
        # [278:5] alias str -> hello
        # [37:5] write(1, address_of(str), array_size_of(str))
        # [37:11] 1
        li a0, 1
        # [37:14] a1 = address_of(str)
        # [37:14] = expression
        # [37:14] address_of(str)
        addi a1, s0, 0
        # [37:31] a2 = array_size_of(str)
        # [37:31] = expression
        # [37:31] array_size_of(str)
        li a2, 21
        li a7, 64
        ecall
    print_278_5_end:
    # [279:5] label
    loop_279_5:
        # [280:9] counter = counter + 1
        # [280:19] counter
        lw t6, 644(s0)
        # [280:29] t6 + 1
        addi t6, t6, 1
        sw t6, 644(s0)
        # [281:9] print_num(counter)
        # [281:9] address of argument 'counter' to parameter 'num'
        addi t6, s0, 644
        sw t6, 776(s0)
        addi sp, sp, -128
        sw a0, 36(sp)
        sw a1, 40(sp)
        sw a2, 44(sp)
        sw a3, 48(sp)
        sw a4, 52(sp)
        sw a5, 56(sp)
        sw a6, 60(sp)
        sw a7, 64(sp)
        sw ra, 0(sp)
        sw gp, 8(sp)
        sw tp, 12(sp)
        sw s0, 28(sp)
        sw s1, 32(sp)
        sw s2, 68(sp)
        sw s3, 72(sp)
        sw s4, 76(sp)
        sw s5, 80(sp)
        sw s6, 84(sp)
        sw s7, 88(sp)
        sw s8, 92(sp)
        sw s9, 96(sp)
        sw s10, 100(sp)
        sw s11, 104(sp)
        sw t0, 16(sp)
        sw t1, 20(sp)
        sw t2, 24(sp)
        sw t3, 108(sp)
        sw t4, 112(sp)
        sw t5, 116(sp)
        sw t6, 120(sp)
        addi s1, s0, 776
        call print_num
        lw a0, 36(sp)
        lw a1, 40(sp)
        lw a2, 44(sp)
        lw a3, 48(sp)
        lw a4, 52(sp)
        lw a5, 56(sp)
        lw a6, 60(sp)
        lw a7, 64(sp)
        lw ra, 0(sp)
        lw gp, 8(sp)
        lw tp, 12(sp)
        lw s0, 28(sp)
        lw s1, 32(sp)
        lw s2, 68(sp)
        lw s3, 72(sp)
        lw s4, 76(sp)
        lw s5, 80(sp)
        lw s6, 84(sp)
        lw s7, 88(sp)
        lw s8, 92(sp)
        lw s9, 96(sp)
        lw s10, 100(sp)
        lw s11, 104(sp)
        lw t0, 16(sp)
        lw t1, 20(sp)
        lw t2, 24(sp)
        lw t3, 108(sp)
        lw t4, 112(sp)
        lw t5, 116(sp)
        lw t6, 120(sp)
        addi sp, sp, 128
        # [282:9] print(colon)
        # [36:6] print(str : i8[])
        print_282_9:
            # [282:9] alias str -> colon
            # [37:5] write(1, address_of(str), array_size_of(str))
            # [37:11] 1
            li a0, 1
            # [37:14] a1 = address_of(str)
            # [37:14] = expression
            # [37:14] address_of(str)
            addi a1, s0, 61
            # [37:31] a2 = array_size_of(str)
            # [37:31] = expression
            # [37:31] array_size_of(str)
            li a2, 2
            li a7, 64
            ecall
        print_282_9_end:
        # [283:9] print(prompt1)
        # [36:6] print(str : i8[])
        print_283_9:
            # [283:9] alias str -> prompt1
            # [37:5] write(1, address_of(str), array_size_of(str))
            # [37:11] 1
            li a0, 1
            # [37:14] a1 = address_of(str)
            # [37:14] = expression
            # [37:14] address_of(str)
            addi a1, s0, 21
            # [37:31] a2 = array_size_of(str)
            # [37:31] = expression
            # [37:31] array_size_of(str)
            li a2, 12
            li a7, 64
            ecall
        print_283_9_end:
        # [284:9] str_in(nm)
        # [72:6] str_in(s : str)
        str_in_284_9:
            # [284:9] alias s -> nm
            # [73:5] var nbytes = read(0, address_of(s.data), array_size_of(s.data))
            # [73:9] nbytes: i32 (4 B @ [s0 + 776])
            # [73:9] nbytes = read(0, address_of(s.data), array_size_of(s.data))
            # [73:18] nbytes = read(0, address_of(s.data), array_size_of(s.data))
            # [73:18] = expression
            # [73:18] read(0, address_of(s.data), array_size_of(s.data))
            # [73:23] 0
            li a0, 0
            # [73:26] a1 = address_of(s.data)
            # [73:26] = expression
            # [73:26] address_of(s.data)
            addi a1, s0, 649
            # [73:46] a2 = array_size_of(s.data)
            # [73:46] = expression
            # [73:46] array_size_of(s.data)
            li a2, 127
            li a7, 63
            ecall
            sw a0, 776(s0)
            # [74:5] s.len = nbytes - 1
            # [74:13] nbytes
            lw t6, 776(s0)
            sb t6, 648(s0)
            # [74:22] s.len - 1
            lb t6, 648(s0)
            addi t6, t6, -1
            sb t6, 648(s0)
        str_in_284_9_end:
        if_285_12:
        # [285:12] ? nm.len == 0
        # [285:12] ? nm.len == 0
        cmp_285_12:
        lb t6, 648(s0)
        bne t6, zero, if_287_19
        1:
        if_285_12_code:
            # [286:13] break
            j loop_279_5_end
        if_287_19:
        # [287:19] ? nm.len <= 4
        # [287:19] ? nm.len <= 4
        cmp_287_19:
        lb t6, 648(s0)
        li t5, 4
        blt t5, t6, if_else_285_9
        1:
        if_287_19_code:
            # [288:13] print(prompt2)
            # [36:6] print(str : i8[])
            print_288_13:
                # [288:13] alias str -> prompt2
                # [37:5] write(1, address_of(str), array_size_of(str))
                # [37:11] 1
                li a0, 1
                # [37:14] a1 = address_of(str)
                # [37:14] = expression
                # [37:14] address_of(str)
                addi a1, s0, 33
                # [37:31] a2 = array_size_of(str)
                # [37:31] = expression
                # [37:31] array_size_of(str)
                li a2, 20
                li a7, 64
                ecall
            print_288_13_end:
            # [289:13] continue
            j loop_279_5
        if_else_285_9:
            # [291:13] print(prompt3)
            # [36:6] print(str : i8[])
            print_291_13:
                # [291:13] alias str -> prompt3
                # [37:5] write(1, address_of(str), array_size_of(str))
                # [37:11] 1
                li a0, 1
                # [37:14] a1 = address_of(str)
                # [37:14] = expression
                # [37:14] address_of(str)
                addi a1, s0, 53
                # [37:31] a2 = array_size_of(str)
                # [37:31] = expression
                # [37:31] array_size_of(str)
                li a2, 6
                li a7, 64
                ecall
            print_291_13_end:
            # [292:13] str_out(nm)
            # [77:6] str_out(s : str)
            str_out_292_13:
                # [292:13] alias s -> nm
                # [78:5] write(1, address_of(s.data), s.len)
                # [78:11] 1
                li a0, 1
                # [78:14] a1 = address_of(s.data)
                # [78:14] = expression
                # [78:14] address_of(s.data)
                addi a1, s0, 649
                # [78:34] s.len
                lb a2, 648(s0)
                li a7, 64
                ecall
            str_out_292_13_end:
            # [293:13] print(dot)
            # [36:6] print(str : i8[])
            print_293_13:
                # [293:13] alias str -> dot
                # [37:5] write(1, address_of(str), array_size_of(str))
                # [37:11] 1
                li a0, 1
                # [37:14] a1 = address_of(str)
                # [37:14] = expression
                # [37:14] address_of(str)
                addi a1, s0, 59
                # [37:31] a2 = array_size_of(str)
                # [37:31] = expression
                # [37:31] array_size_of(str)
                li a2, 1
                li a7, 64
                ecall
            print_293_13_end:
            # [294:13] print(nl)
            # [36:6] print(str : i8[])
            print_294_13:
                # [294:13] alias str -> nl
                # [37:5] write(1, address_of(str), array_size_of(str))
                # [37:11] 1
                li a0, 1
                # [37:14] a1 = address_of(str)
                # [37:14] = expression
                # [37:14] address_of(str)
                addi a1, s0, 60
                # [37:31] a2 = array_size_of(str)
                # [37:31] = expression
                # [37:31] array_size_of(str)
                li a2, 1
                li a7, 64
                ecall
            print_294_13_end:
        if_285_9_end:
    j loop_279_5
    loop_279_5_end:
    li a0, 0
    li a7, 93
    ecall
# 
# [98:15] noinline print_num(num)
print_num:
    addi sp, sp, -16
    sw ra, 0(sp)
    # [98:25] num: i32 (4 B @ [s1])
    # [99:5] var buf : i8[20]
    # [99:9] buf: i8[20] (20 B @ [s1 + 4])
    # [99:9] zero 20 * 1 B = 20 B
    addi t6, s1, 4
    li t5, 5
    1:
    sw zero, 0(t6)
    addi t6, t6, 4
    addi t5, t5, -1
    bnez t5, 1b
    # [100:5] var n = num
    # [100:9] n: i32 (4 B @ [s1 + 24])
    # [100:9] n = num
    # [100:13] num
    lw t6, 0(s1)
    lw t5, 0(t6)
    sw t5, 24(s1)
    # [101:5] var is_negative : bool = false
    # [101:9] is_negative: bool (1 B @ [s1 + 28])
    # [101:9] is_negative = false
    sb zero, 28(s1)
    if_103_8:
    # [103:8] ? n < 0
    # [103:8] ? n < 0
    cmp_103_8:
    lw t6, 24(s1)
    bge t6, zero, if_103_5_end
    1:
    if_103_8_code:
        # [104:9] is_negative = true
        li t6, 1
        sb t6, 28(s1)
        # [105:9] n = -n
        # [105:14] -n
        lw t6, 24(s1)
        sw t6, 24(s1)
        lw t6, 24(s1)
        sub t6, zero, t6
        sw t6, 24(s1)
    if_103_5_end:
    # [108:5] var i = 20
    # [108:9] i: i32 (4 B @ [s1 + 29])
    # [108:9] i = 20
    # [108:13] 20
    li t6, 20
    sw t6, 29(s1)
    # [109:5] label
    loop_109_5:
        # [110:9] i = i - 1
        # [110:13] i
        lw t6, 29(s1)
        # [110:17] t6 - 1
        addi t6, t6, -1
        sw t6, 29(s1)
        # [111:9] var ascii = 48 + (n % 10)
        # [111:13] ascii: i32 (4 B @ [s1 + 33])
        # [111:13] ascii = 48 + (n % 10)
        # [111:21] 48
        li t6, 48
        # [111:27] t6 + (n % 10)
        # [111:27] n
        lw t5, 24(s1)
        # [111:31] t5 % 10
        # [111:31] div const
        addi a0, t5, 0
        li a1, 10
        call .Lbaz_divide
        addi t5, a1, 0
        add t6, t6, t5
        sw t6, 33(s1)
        # [112:9] # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
        # [113:9] buf[i] = ascii
        addi t6, s1, 4
        # [113:13] set array index
        # [113:13] i
        lw t5, 29(s1)
        add t6, t6, t5
        # [113:18] ascii
        lw t5, 33(s1)
        sb t5, 0(t6)
        # [114:9] n = n / 10
        # [114:13] n
        lw t6, 24(s1)
        sw t6, 24(s1)
        # [114:17] n / 10
        # [114:17] div const
        lw a0, 24(s1)
        li a1, 10
        call .Lbaz_divide
        sw a0, 24(s1)
        if_115_12:
        # [115:12] ? n == 0
        # [115:12] ? n == 0
        cmp_115_12:
        lw t6, 24(s1)
        bne t6, zero, if_115_9_end
        1:
        if_115_12_code:
            # [115:19] break
            j loop_109_5_end
        if_115_9_end:
    j loop_109_5
    loop_109_5_end:
    if_118_8:
    # [118:8] ? is_negative
    # [118:8] ? is_negative
    cmp_118_8:
    lbu t6, 28(s1)
    beq t6, zero, if_118_5_end
    1:
    if_118_8_code:
        # [119:9] i = i - 1
        # [119:13] i
        lw t6, 29(s1)
        # [119:17] t6 - 1
        addi t6, t6, -1
        sw t6, 29(s1)
        # [120:9] buf[i] = 45
        addi t6, s1, 4
        # [120:13] set array index
        # [120:13] i
        lw t5, 29(s1)
        add t6, t6, t5
        # [120:18] 45
        li t5, 45
        sb t5, 0(t6)
    if_118_5_end:
    # [123:5] var write_pos = 0
    # [123:9] write_pos: i32 (4 B @ [s1 + 33])
    # [123:9] write_pos = 0
    # [123:21] 0
    sw zero, 33(s1)
    # [124:5] label
    loop_124_5:
        # [125:9] buf[write_pos] = buf[i]
        addi t6, s1, 4
        # [125:13] set array index
        # [125:13] write_pos
        lw t5, 33(s1)
        add t6, t6, t5
        # [125:26] buf[i]
        addi t5, s1, 4
        # [125:30] set array index
        # [125:30] i
        lw t4, 29(s1)
        add t5, t5, t4
        lb t4, 0(t5)
        sb t4, 0(t6)
        # [126:9] write_pos = write_pos + 1
        # [126:21] write_pos
        lw t6, 33(s1)
        # [126:33] t6 + 1
        addi t6, t6, 1
        sw t6, 33(s1)
        # [127:9] i = i + 1
        # [127:13] i
        lw t6, 29(s1)
        # [127:17] t6 + 1
        addi t6, t6, 1
        sw t6, 29(s1)
        if_128_12:
        # [128:12] ? i == 20
        # [128:12] ? i == 20
        cmp_128_12:
        lw t6, 29(s1)
        li t5, 20
        bne t6, t5, if_128_9_end
        1:
        if_128_12_code:
            # [128:20] break
            j loop_124_5_end
        if_128_9_end:
    j loop_124_5
    loop_124_5_end:
    # [131:5] write(1, address_of(buf), write_pos)
    # [131:11] 1
    li a0, 1
    # [131:14] a1 = address_of(buf)
    # [131:14] = expression
    # [131:14] address_of(buf)
    addi a1, s1, 4
    # [131:31] write_pos
    lw a2, 33(s1)
    li a7, 64
    ecall
    lw ra, 0(sp)
    addi sp, sp, 16
    ret
.equ print_num.size, 37
.Lbaz_divide:
    beqz a1, 5f
    srai t2, a0, 31
    srai t1, a1, 31
    xor a0, a0, t2
    sub a0, a0, t2
    xor a1, a1, t1
    sub a1, a1, t1
    xor t1, t1, t2
    li t0, 0
    li t3, 32
1:
    srli t4, a0, 31
    slli t0, t0, 1
    or t0, t0, t4
    slli a0, a0, 1
    bltu t0, a1, 2f
    sub t0, t0, a1
    ori a0, a0, 1
2:
    addi t3, t3, -1
    bnez t3, 1b
    xor a0, a0, t1
    sub a0, a0, t1
    xor a1, t0, t2
    sub a1, a1, t2
    ret
5:
    ebreak
    j 5b
.data
.balign 16
dat:
# [20:7] hello
# [20:22] i8[21]
.ascii "hello world from baz\n"
# [21:5] prompt1
# [21:22] i8[12]
.ascii "enter name:\n"
# [22:5] prompt2
# [22:22] i8[20]
.ascii "that is not a name.\n"
# [23:5] prompt3
# [23:22] i8[6]
.ascii "hello "
# [24:9] dot
# [24:22] i8[1]
.ascii "."
# [25:10] nl
# [25:22] i8[1]
.ascii "\n"
# [26:7] colon
# [26:22] i8[2]
.ascii ": "
# [27:8] nums
# i32[4]
# [27:23] [0]
# [27:23] i32
.rept 1
.word 1
.endr
# pad 3 'i32' of size 4
.zero 12
# [28:8] str1
# [28:23] i8
.rept 1
.byte 3
.endr
# [28:21] zero remaining fields
.zero 127
dat.end:
.balign 16
vars:
.zero 65536
vars.end:
#            max frames in use: 8
#              dat var padding: 1 B
#                max vars size: 572 B
```
