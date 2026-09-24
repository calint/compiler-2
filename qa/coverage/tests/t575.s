# [4:9] buf: i8[4] (4 B @ [s0])
# [5:9] a: i8[4] (4 B @ [s0 + 4])
# [6:9] k: i32 (4 B @ [s0 + 8])
# [7:9] n: i32 (4 B @ [s0 + 12])
# allocate named register s0
.option norvc
.option norelax
.text
.globl _start
_start:
    la s0, dat
# [1:1] # a negative 'array_copy' count with start elements panics
# 
main:
    # [4:5] var buf[4] i8
    # [4:9] buf: i8[4] (4 B @ [s0])
    # [4:9] zero 4 * 1 B = 4 B
    sw zero, 0(s0)
    # [5:5] var a[4] i8
    # [5:9] a: i8[4] (4 B @ [s0 + 4])
    # [5:9] zero 4 * 1 B = 4 B
    sw zero, 4(s0)
    # [6:5] var k = 1
    # [6:9] k: i32 (4 B @ [s0 + 8])
    # [6:9] k = 1
    # [6:13] 1
    # [6:13] allocate scratch register -> t0
    li t0, 1
    sw t0, 8(s0)
    # [6:13] free scratch register t0
    # [7:5] var n = -1
    # [7:9] n: i32 (4 B @ [s0 + 12])
    # [7:9] n = -1
    # [7:14] -1
    # [7:14] allocate scratch register -> t0
    li t0, -1
    sw t0, 12(s0)
    # [7:14] free scratch register t0
    # [8:5] array_copy(a[k], buf[k], n)
    # [8:5] allocate scratch register -> t0
    # [8:5] allocate scratch register -> t1
    # [8:5] allocate scratch register -> t2
    # [8:5] t0: source, t1: destination, t2: count
    # [8:30] n
    # [8:30] n
    lw t2, 12(s0)
    # [8:16] a[k]
    # [8:18] allocate scratch register -> t3
    # [8:18] set array index
    # [8:18] k
    lw t3, 8(s0)
    # [8:18] bounds check
    bltz t3, 1f
    # [8:18] allocate scratch register -> t4
    # [8:18] allocate scratch register -> t5
    # [8:18] allocate scratch register -> t6
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
    li a0, 8
    j baz_bounds_panic
    2:
    # [8:18] free scratch register t6
    # [8:18] free scratch register t5
    # [8:18] free scratch register t4
    add t0, s0, t3
    addi t0, t0, 4
    # [8:5] free scratch register t3
    # [8:22] buf[k]
    # [8:26] allocate scratch register -> t3
    # [8:26] set array index
    # [8:26] k
    lw t3, 8(s0)
    # [8:26] bounds check
    bltz t3, 1f
    # [8:26] allocate scratch register -> t4
    # [8:26] allocate scratch register -> t5
    # [8:26] allocate scratch register -> t6
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
    li a0, 8
    j baz_bounds_panic
    2:
    # [8:26] free scratch register t6
    # [8:26] free scratch register t5
    # [8:26] free scratch register t4
    add t1, s0, t3
    # [8:5] free scratch register t3
    # [8:5] t2: elements to bytes (1 bytes/element)
    # [8:5] allocate scratch register -> t3
    # [8:5] allocate scratch register -> t4
    # [8:5] t3: copy value, t4: words, t2: tail bytes
    # [8:5] split bytes into words and tail; skip word loop if none
    srli t4, t2, 2
    andi t2, t2, 3
    beqz t4, 2f
    # [8:5] copy 4-byte words
    1:
    lw t3, 0(t0)
    sw t3, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    addi t4, t4, -1
    bnez t4, 1b
    2:
    # [8:5] copy optional 2-byte tail
    andi t3, t2, 2
    beqz t3, 3f
    lhu t3, 0(t0)
    sh t3, 0(t1)
    addi t0, t0, 2
    addi t1, t1, 2
    3:
    # [8:5] copy optional final byte
    andi t2, t2, 1
    beqz t2, 4f
    lbu t3, 0(t0)
    sb t3, 0(t1)
    4:
    # [8:5] free scratch register t4
    # [8:5] free scratch register t3
    # [8:5] free scratch register t2
    # [8:5] free scratch register t1
    # [8:5] free scratch register t0
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
#            max frames in use: 3
#              dat var padding: 0 B
#                max vars size: 16 B
