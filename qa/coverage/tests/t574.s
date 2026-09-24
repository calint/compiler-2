# [4:9] buf: i8[4] (4 B @ [s0])
# [5:9] a: i8[4] (4 B @ [s0 + 4])
# [6:9] k: i32 (4 B @ [s0 + 8])
# [7:9] n: i32 (4 B @ [s0 + 12])
# [8:9] r: i32 (4 B @ [s0 + 16])
# allocate named register s0
.option norvc
.option norelax
.text
.globl _start
_start:
    la s0, dat
# [1:1] # a negative count with a start panics
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
    # [6:5] var k = 0
    # [6:9] k: i32 (4 B @ [s0 + 8])
    # [6:9] k = 0
    # [6:13] 0
    sw zero, 8(s0)
    # [7:5] var n = -1
    # [7:9] n: i32 (4 B @ [s0 + 12])
    # [7:9] n = -1
    # [7:14] -1
    # [7:14] allocate scratch register -> t0
    li t0, -1
    sw t0, 12(s0)
    # [7:14] free scratch register t0
    # [8:5] var r = write(1, buf, n, k)
    # [8:9] r: i32 (4 B @ [s0 + 16])
    # [8:9] r = write(1, buf, n, k)
    # [8:13] r = write(1, buf, n, k)
    # [8:13] = expression
    # [8:13] write(1, buf, n, k)
    # [8:13] allocate named register a0
    # [8:13] allocate named register a1
    # [8:13] allocate named register a2
    # [8:19] 1
    li a0, 1
    # [8:27] n
    lw a2, 12(s0)
    # [8:30] allocate scratch register -> t0
    # [8:30] k
    lw t0, 8(s0)
    # [8:30] bounds check
    bltz t0, 1f
    # [8:30] allocate scratch register -> t1
    # [8:30] allocate scratch register -> t2
    # [8:30] allocate scratch register -> t3
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
    li a0, 8
    j baz_bounds_panic
    2:
    # [8:30] free scratch register t3
    # [8:30] free scratch register t2
    # [8:30] free scratch register t1
    addi a1, s0, 0
    add a1, a1, t0
    # [8:13] free scratch register t0
    # [8:13] allocate named register a7
    li a7, 64
    ecall
    # [8:13] free named register a7
    sw a0, 16(s0)
    # [8:13] free named register a2
    # [8:13] free named register a1
    # [8:13] free named register a0
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
#                max vars size: 20 B
