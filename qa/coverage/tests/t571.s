# [3:13] err: i32 (4 B @ [s0])
# [3:18] expr: bool (1 B @ [s0 + 4])
# [6:9] buf: i8[4] (4 B @ [s0])
# [7:9] n: i32 (4 B @ [s0 + 4])
# [11:9] a: i8[4] (4 B @ [s0 + 8])
# [12:9] b: i8[4] (4 B @ [s0 + 12])
# [13:9] k: i32 (4 B @ [s0 + 16])
# allocate named register s0
.option norvc
.option norelax
.text
.globl _start
_start:
    la s0, dat
# [1:1] # a range may end exactly at the array end, an empty range may start there
# 
main:
    # [6:5] var buf[4] i8
    # [6:9] buf: i8[4] (4 B @ [s0])
    # [6:9] zero 4 * 1 B = 4 B
    sw zero, 0(s0)
    # [7:5] var n = read(0, buf, 2, 2)
    # [7:9] n: i32 (4 B @ [s0 + 4])
    # [7:9] n = read(0, buf, 2, 2)
    # [7:13] n = read(0, buf, 2, 2)
    # [7:13] = expression
    # [7:13] read(0, buf, 2, 2)
    # [7:13] allocate named register a0
    # [7:13] allocate named register a1
    # [7:13] allocate named register a2
    # [7:18] 0
    li a0, 0
    # [7:26] 2
    li a2, 2
    # [7:29] allocate scratch register -> t0
    # [7:29] 2
    li t0, 2
    # [7:29] bounds check
    bltz t0, 1f
    # [7:29] allocate scratch register -> t1
    # [7:29] allocate scratch register -> t2
    # [7:29] allocate scratch register -> t3
    srai t3, t0, 31
    srai t1, a2, 31
    add t3, t3, t1
    add t2, t0, a2
    sltu t1, t2, t0
    add t3, t3, t1
    bltz t3, 2f
    bgtz t3, 1f
    li t1, 4
    bgeu t1, t2, 2f
    1:
    li a0, 7
    j baz_bounds_panic
    2:
    # [7:29] free scratch register t3
    # [7:29] free scratch register t2
    # [7:29] free scratch register t1
    addi a1, s0, 0
    add a1, a1, t0
    # [7:13] free scratch register t0
    # [7:13] allocate named register a7
    li a7, 63
    ecall
    # [7:13] free named register a7
    sw a0, 4(s0)
    # [7:13] free named register a2
    # [7:13] free named register a1
    # [7:13] free named register a0
    # [8:5] assert(1, n == 0)
    # [8:15] allocate scratch register -> t0
    # [8:15] ? n == 0
    # [8:15] ? n == 0
    cmp.8.15:
    lw t0, 4(s0)
    sltiu t0, t0, 1
    1:
    bool.8.15.end:
    # [3:6] assert(err, expr bool)
    func.assert.8.5:
        # [8:5] alias err -> 1
        # [8:5] alias expr -> t0
        if.3.34.8.5:
        # [3:34] ? not expr
        # [3:34] ? not expr
        cmp.3.34.8.5:
        bne t0, zero, if.3.31.8.5.end
        1:
        if.3.34.8.5.code:
            # [3:43] exit(err)
            # [3:43] allocate named register a0
            # [3:48] err
            li a0, 1
            li a7, 93
            ecall
            # [3:43] free named register a0
        if.3.31.8.5.end:
        # [8:5] free scratch register t0
    func.assert.8.5.end:
    # [9:5] n = read(0, buf, 0, 4)
    # [9:9] n = read(0, buf, 0, 4)
    # [9:9] = expression
    # [9:9] read(0, buf, 0, 4)
    # [9:9] allocate named register a0
    # [9:9] allocate named register a1
    # [9:9] allocate named register a2
    # [9:14] 0
    li a0, 0
    # [9:22] 0
    li a2, 0
    # [9:25] allocate scratch register -> t0
    # [9:25] 4
    li t0, 4
    # [9:25] bounds check
    bltz t0, 1f
    # [9:25] allocate scratch register -> t1
    # [9:25] allocate scratch register -> t2
    # [9:25] allocate scratch register -> t3
    srai t3, t0, 31
    srai t1, a2, 31
    add t3, t3, t1
    add t2, t0, a2
    sltu t1, t2, t0
    add t3, t3, t1
    bltz t3, 2f
    bgtz t3, 1f
    li t1, 4
    bgeu t1, t2, 2f
    1:
    li a0, 9
    j baz_bounds_panic
    2:
    # [9:25] free scratch register t3
    # [9:25] free scratch register t2
    # [9:25] free scratch register t1
    addi a1, s0, 0
    add a1, a1, t0
    # [9:9] free scratch register t0
    # [9:9] allocate named register a7
    li a7, 63
    ecall
    # [9:9] free named register a7
    sw a0, 4(s0)
    # [9:9] free named register a2
    # [9:9] free named register a1
    # [9:9] free named register a0
    # [10:5] assert(2, n == 0)
    # [10:15] allocate scratch register -> t0
    # [10:15] ? n == 0
    # [10:15] ? n == 0
    cmp.10.15:
    lw t0, 4(s0)
    sltiu t0, t0, 1
    1:
    bool.10.15.end:
    # [3:6] assert(err, expr bool)
    func.assert.10.5:
        # [10:5] alias err -> 2
        # [10:5] alias expr -> t0
        if.3.34.10.5:
        # [3:34] ? not expr
        # [3:34] ? not expr
        cmp.3.34.10.5:
        bne t0, zero, if.3.31.10.5.end
        1:
        if.3.34.10.5.code:
            # [3:43] exit(err)
            # [3:43] allocate named register a0
            # [3:48] err
            li a0, 2
            li a7, 93
            ecall
            # [3:43] free named register a0
        if.3.31.10.5.end:
        # [10:5] free scratch register t0
    func.assert.10.5.end:
    # [11:5] var a[4] i8 = {1, 2, 3, 4}
    # [11:9] a: i8[4] (4 B @ [s0 + 8])
    # [11:9] a= {1, 2, 3, 4}
    # [11:20] [0]
    # [11:20] 1
    # [11:20] allocate scratch register -> t0
    li t0, 1
    sb t0, 8(s0)
    # [11:20] free scratch register t0
    # [11:20] [1]
    # [11:23] 2
    # [11:23] allocate scratch register -> t0
    li t0, 2
    sb t0, 9(s0)
    # [11:23] free scratch register t0
    # [11:20] [2]
    # [11:26] 3
    # [11:26] allocate scratch register -> t0
    li t0, 3
    sb t0, 10(s0)
    # [11:26] free scratch register t0
    # [11:20] [3]
    # [11:29] 4
    # [11:29] allocate scratch register -> t0
    li t0, 4
    sb t0, 11(s0)
    # [11:29] free scratch register t0
    # [12:5] var b[4] i8
    # [12:9] b: i8[4] (4 B @ [s0 + 12])
    # [12:9] zero 4 * 1 B = 4 B
    sw zero, 12(s0)
    # [13:5] var k = 2
    # [13:9] k: i32 (4 B @ [s0 + 16])
    # [13:9] k = 2
    # [13:13] 2
    # [13:13] allocate scratch register -> t0
    li t0, 2
    sw t0, 16(s0)
    # [13:13] free scratch register t0
    # [14:5] array_copy(a[k], b[k], 2)
    # [14:5] allocate scratch register -> t0
    # [14:5] allocate scratch register -> t1
    # [14:5] allocate scratch register -> t2
    # [14:5] t0: source, t1: destination, t2: count
    # [14:28] 2
    # [14:28] 2
    li t2, 2
    # [14:16] a[k]
    # [14:18] allocate scratch register -> t3
    # [14:18] set array index
    # [14:18] k
    lw t3, 16(s0)
    # [14:18] bounds check
    bltz t3, 1f
    # [14:18] allocate scratch register -> t4
    # [14:18] allocate scratch register -> t5
    # [14:18] allocate scratch register -> t6
    srai t6, t3, 31
    srai t4, t2, 31
    add t6, t6, t4
    add t5, t3, t2
    sltu t4, t5, t3
    add t6, t6, t4
    bltz t6, 2f
    bgtz t6, 1f
    li t4, 4
    bgeu t4, t5, 2f
    1:
    li a0, 14
    j baz_bounds_panic
    2:
    # [14:18] free scratch register t6
    # [14:18] free scratch register t5
    # [14:18] free scratch register t4
    add t0, s0, t3
    addi t0, t0, 8
    # [14:5] free scratch register t3
    # [14:22] b[k]
    # [14:24] allocate scratch register -> t3
    # [14:24] set array index
    # [14:24] k
    lw t3, 16(s0)
    # [14:24] bounds check
    bltz t3, 1f
    # [14:24] allocate scratch register -> t4
    # [14:24] allocate scratch register -> t5
    # [14:24] allocate scratch register -> t6
    srai t6, t3, 31
    srai t4, t2, 31
    add t6, t6, t4
    add t5, t3, t2
    sltu t4, t5, t3
    add t6, t6, t4
    bltz t6, 2f
    bgtz t6, 1f
    li t4, 4
    bgeu t4, t5, 2f
    1:
    li a0, 14
    j baz_bounds_panic
    2:
    # [14:24] free scratch register t6
    # [14:24] free scratch register t5
    # [14:24] free scratch register t4
    add t1, s0, t3
    addi t1, t1, 12
    # [14:5] free scratch register t3
    # [14:5] t2: elements to bytes (1 bytes/element)
    # [14:5] allocate scratch register -> t3
    # [14:5] allocate scratch register -> t4
    # [14:5] t3: copy value, t4: words, t2: tail bytes
    # [14:5] split bytes into words and tail; skip word loop if none
    srli t4, t2, 2
    andi t2, t2, 3
    beqz t4, 2f
    # [14:5] copy 4-byte words
    1:
    lw t3, 0(t0)
    sw t3, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    addi t4, t4, -1
    bnez t4, 1b
    2:
    # [14:5] copy optional 2-byte tail
    andi t3, t2, 2
    beqz t3, 3f
    lhu t3, 0(t0)
    sh t3, 0(t1)
    addi t0, t0, 2
    addi t1, t1, 2
    3:
    # [14:5] copy optional final byte
    andi t2, t2, 1
    beqz t2, 4f
    lbu t3, 0(t0)
    sb t3, 0(t1)
    4:
    # [14:5] free scratch register t4
    # [14:5] free scratch register t3
    # [14:5] free scratch register t2
    # [14:5] free scratch register t1
    # [14:5] free scratch register t0
    # [15:5] assert(3, b[3] == 4)
    # [15:15] allocate scratch register -> t0
    # [15:15] ? b[3] == 4
    # [15:15] ? b[3] == 4
    cmp.15.15:
    # [15:17] allocate scratch register -> t1
    # [15:17] set array index
    # [15:17] 3
    li t1, 3
    # [15:17] bounds check
    bltz t1, 1f
    # [15:17] allocate scratch register -> t2
    bltz t1, 2f
    li t2, 4
    bltu t1, t2, 2f
    1:
    li a0, 15
    j baz_bounds_panic
    2:
    # [15:17] free scratch register t2
    add t0, s0, t1
    lb t0, 12(t0)
    xori t0, t0, 4
    sltiu t0, t0, 1
    1:
    # [15:15] free scratch register t1
    bool.15.15.end:
    # [3:6] assert(err, expr bool)
    func.assert.15.5:
        # [15:5] alias err -> 3
        # [15:5] alias expr -> t0
        if.3.34.15.5:
        # [3:34] ? not expr
        # [3:34] ? not expr
        cmp.3.34.15.5:
        bne t0, zero, if.3.31.15.5.end
        1:
        if.3.34.15.5.code:
            # [3:43] exit(err)
            # [3:43] allocate named register a0
            # [3:48] err
            li a0, 3
            li a7, 93
            ecall
            # [3:43] free named register a0
        if.3.31.15.5.end:
        # [15:5] free scratch register t0
    func.assert.15.5.end:
    # [16:5] assert(4, arrays_equal(a[k], b[k], 2))
    # [16:15] allocate scratch register -> t0
    # [16:15] ? arrays_equal(a[k], b[k], 2)
    # [16:15] ? arrays_equal(a[k], b[k], 2)
    cmp.16.15:
        # [16:15] arrays_equal(a[k], b[k], 2)
        # [16:15] allocate scratch register -> t1
        # [16:15] allocate scratch register -> t2
        # [16:15] allocate scratch register -> t3
        # [16:15] t1: source, t2: destination, t3: count
        # [16:40] 2
        # [16:40] 2
        li t3, 2
        # [16:28] a[k]
        # [16:30] allocate scratch register -> t4
        # [16:30] set array index
        # [16:30] k
        lw t4, 16(s0)
        # [16:30] bounds check
        bltz t4, 1f
        # [16:30] allocate scratch register -> t5
        # [16:30] allocate scratch register -> t6
        # [16:30] allocate scratch register -> s1
        srai s1, t4, 31
        srai t5, t3, 31
        add s1, s1, t5
        add t6, t4, t3
        sltu t5, t6, t4
        add s1, s1, t5
        bltz s1, 2f
        bgtz s1, 1f
        li t5, 4
        bgeu t5, t6, 2f
        1:
        li a0, 16
        j baz_bounds_panic
        2:
        # [16:30] free scratch register s1
        # [16:30] free scratch register t6
        # [16:30] free scratch register t5
        add t1, s0, t4
        addi t1, t1, 8
        # [16:15] free scratch register t4
        # [16:34] b[k]
        # [16:36] allocate scratch register -> t4
        # [16:36] set array index
        # [16:36] k
        lw t4, 16(s0)
        # [16:36] bounds check
        bltz t4, 1f
        # [16:36] allocate scratch register -> t5
        # [16:36] allocate scratch register -> t6
        # [16:36] allocate scratch register -> s1
        srai s1, t4, 31
        srai t5, t3, 31
        add s1, s1, t5
        add t6, t4, t3
        sltu t5, t6, t4
        add s1, s1, t5
        bltz s1, 2f
        bgtz s1, 1f
        li t5, 4
        bgeu t5, t6, 2f
        1:
        li a0, 16
        j baz_bounds_panic
        2:
        # [16:36] free scratch register s1
        # [16:36] free scratch register t6
        # [16:36] free scratch register t5
        add t2, s0, t4
        addi t2, t2, 12
        # [16:15] free scratch register t4
        # [16:15] t3: elements to bytes (1 bytes/element)
        # [16:15] allocate scratch register -> t4
        # [16:15] allocate scratch register -> t5
        # [16:15] t0: left value/result, t5: right value, t4: words, t3: tail bytes
        # [16:15] stop at first mismatch
        # [16:15] split bytes into words and tail; skip word loop if none
        srli t4, t3, 2
        andi t3, t3, 3
        beqz t4, 2f
        # [16:15] compare 4-byte words
        1:
        lw t0, 0(t1)
        lw t5, 0(t2)
        bne t0, t5, 5f
        addi t1, t1, 4
        addi t2, t2, 4
        addi t4, t4, -1
        bnez t4, 1b
        2:
        # [16:15] compare optional 2-byte tail
        andi t0, t3, 2
        beqz t0, 3f
        lhu t0, 0(t1)
        lhu t5, 0(t2)
        bne t0, t5, 5f
        addi t1, t1, 2
        addi t2, t2, 2
        3:
        # [16:15] compare optional final byte
        andi t3, t3, 1
        beqz t3, 4f
        lbu t0, 0(t1)
        lbu t5, 0(t2)
        bne t0, t5, 5f
        4:
        # [16:15] all matched or empty: true
        li t0, 1
        j 6f
        5:
        # [16:15] mismatch: false
        li t0, 0
        6:
        # [16:15] free scratch register t5
        # [16:15] free scratch register t4
        # [16:15] free scratch register t3
        # [16:15] free scratch register t2
        # [16:15] free scratch register t1
    bool.16.15.end:
    # [3:6] assert(err, expr bool)
    func.assert.16.5:
        # [16:5] alias err -> 4
        # [16:5] alias expr -> t0
        if.3.34.16.5:
        # [3:34] ? not expr
        # [3:34] ? not expr
        cmp.3.34.16.5:
        bne t0, zero, if.3.31.16.5.end
        1:
        if.3.34.16.5.code:
            # [3:43] exit(err)
            # [3:43] allocate named register a0
            # [3:48] err
            li a0, 4
            li a7, 93
            ecall
            # [3:43] free named register a0
        if.3.31.16.5.end:
        # [16:5] free scratch register t0
    func.assert.16.5.end:
    li a0, 0
    li a7, 93
    ecall
# 
baz_bounds_panic:
    mv s2, a0
    li a0, 2
    la a1, .Lbaz_bounds_message
    li a2, 22
    li a7, 64
    ecall
    addi sp, sp, -16
    mv a1, sp
    li a2, 0
    la t0, .Lbaz_decimal_places
1:
    lw t1, 0(t0)
    li t2, 0
2:
    bltu s2, t1, 3f
    sub s2, s2, t1
    addi t2, t2, 1
    j 2b
3:
    or t3, a2, t2
    bnez t3, 4f
    li t3, 1
    bne t1, t3, 5f
4:
    addi t2, t2, 48
    sb t2, 0(a1)
    addi a1, a1, 1
    addi a2, a2, 1
5:
    addi t0, t0, 4
    li t3, 1
    bne t1, t3, 1b
    li t2, 10
    sb t2, 0(a1)
    addi a2, a2, 1
    mv a1, sp
    li a0, 2
    li a7, 64
    ecall
    li a0, 255
    li a7, 93
    ecall
.section .rodata
.Lbaz_bounds_message:
.ascii "panic: bounds at line "
.balign 4
.Lbaz_decimal_places:
.word 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1
.text
.data
.balign 16
dat:
dat.end:
.balign 16
vars:
.zero 65536
vars.end:
# free named register s0
#            max frames in use: 6
#              dat var padding: 0 B
#                max vars size: 20 B
