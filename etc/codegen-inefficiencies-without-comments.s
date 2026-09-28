.option norvc
.option norelax
.text
.globl _start
_start:
la s0, dat
lui sp, 2048
main:
    li t0, 1
    sw t0, 0(s0)
    sw zero, 4(s0)
    sw zero, 8(s0)
    sw zero, 12(s0)
    sw zero, 16(s0)
    li t0, 3
    sw t0, 20(s0)
    li t0, 5
    sw t0, 24(s0)
    sw zero, 28(s0)
    sw zero, 32(s0)
    sw zero, 36(s0)
    sw zero, 40(s0)
    sw zero, 44(s0)
    sw zero, 48(s0)
    addi t0, s0, 52
    li t1, 64
    1:
    sw zero, 0(t0)
    addi t0, t0, 4
    addi t1, t1, -1
    bnez t1, 1b
    cmp.72.12:
        lw t1, 20(s0)
        addi t1, t1, 2
    xori t0, t1, 5
    sltiu t0, t0, 1
    bool.72.12.end:
    func.assert.72.5:
        if.28.27.72.5:
        cmp.28.27.72.5:
        bne t0, zero, if.28.24.72.5.end
        if.28.27.72.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.72.5.end:
    func.assert.72.5.end:
    cmp.76.12:
        li t1, 2
    xori t0, t1, 2
    sltiu t0, t0, 1
    bool.76.12.end:
    func.assert.76.5:
        if.28.27.76.5:
        cmp.28.27.76.5:
        bne t0, zero, if.28.24.76.5.end
        if.28.27.76.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.76.5.end:
    func.assert.76.5.end:
    li t0, 2
    sw t0, 8(s0)
    cmp.80.12:
    lw t0, 8(s0)
    xori t0, t0, 2
    sltiu t0, t0, 1
    bool.80.12.end:
    func.assert.80.5:
        if.28.27.80.5:
        cmp.28.27.80.5:
        bne t0, zero, if.28.24.80.5.end
        if.28.27.80.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.80.5.end:
    func.assert.80.5.end:
    li t0, 74
    sw t0, 44(s0)
    cmp.85.12:
    lw t0, 44(s0)
    xori t0, t0, 74
    sltiu t0, t0, 1
    bool.85.12.end:
    func.assert.85.5:
        if.28.27.85.5:
        cmp.28.27.85.5:
        bne t0, zero, if.28.24.85.5.end
        if.28.27.85.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.85.5.end:
    func.assert.85.5.end:
    li t0, 65518
    sw t0, 88(s0)
    li t0, 1
    sw t0, 308(s0)
    lw t0, 308(s0)
    addi t0, t0, 1
    lw t1, 308(s0)
    slli t2, t1, 2
    add t2, t2, s0
    lw t2, 4(t2)
    slli t3, t0, 2
    add t3, t3, s0
    sw t2, 4(t3)
    lw t0, 308(s0)
    slli t1, t0, 5
    add t1, t1, s0
    li t2, 65518
    sw t2, 56(t1)
    lw t0, 0(s0)
    sub t0, zero, t0
    sw t0, 312(s0)
    lw t0, 0(s0)
    xori t0, t0, -1
    sw t0, 312(s0)
    sb zero, 316(s0)
    lw t0, 0(s0)
    addi t0, t0, -1
    sb t0, 316(s0)
    lw t0, 0(s0)
    sub t0, zero, t0
    sw t0, 320(s0)
    lw t0, 312(s0)
    sub t0, zero, t0
    sw t0, 324(s0)
    lw t0, 308(s0)
    lw t2, 308(s0)
    addi t2, t2, -1
    func.inv.116.16:
        slli t1, t2, 2
        add t1, t1, s0
        lw t1, 4(t1)
        xori t1, t1, -1
    func.inv.116.16.end:
    xori t1, t1, -1
    slli t2, t0, 2
    add t2, t2, s0
    sw t1, 4(t2)
    cmp.120.12:
    lw t0, 0(s0)
    xori t0, t0, 1
    sltiu t0, t0, 1
    bool.120.12.end:
    func.assert.120.5:
        if.28.27.120.5:
        cmp.28.27.120.5:
        bne t0, zero, if.28.24.120.5.end
        if.28.27.120.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.120.5.end:
    func.assert.120.5.end:
    cmp.123.19:
    lw t0, 0(s0)
    xori t0, t0, 1
    sltiu t0, t0, 1
    sb t0, 328(s0)
    bool.123.19.end:
    cmp.124.12:
    lbu t0, 328(s0)
    bool.124.12.end:
    func.assert.124.5:
        if.28.27.124.5:
        cmp.28.27.124.5:
        bne t0, zero, if.28.24.124.5.end
        if.28.27.124.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.124.5.end:
    func.assert.124.5.end:
    lw t0, 0(s0)
    slli t1, t0, 2
    add t0, t1, t0
    slli t0, t0, 1
    sw t0, 312(s0)
    lw t0, 308(s0)
    slli t1, t0, 2
    add t1, t1, s0
    lw t1, 4(t1)
    sw t1, 312(s0)
    lw t0, 308(s0)
    slli t1, t0, 2
    sub t0, t1, t0
    slli t0, t0, 2
    add t1, s0, t0
    func.object_init.136.14:
        li t2, 2
        sw t2, 28(t1)
        li t2, 74
        sw t2, 32(t1)
        li t2, 16777215
        sw t2, 36(t1)
    func.object_init.136.14.end:
    sb zero, 329(s0)
    sh zero, 330(s0)
    sw zero, 332(s0)
    sw zero, 336(s0)
    sw zero, 340(s0)
    sw zero, 344(s0)
    sb zero, 348(s0)
    lw t0, 320(s0)
    sw t0, 352(s0)
    lw t0, 324(s0)
    sw t0, 356(s0)
    cmp.147.12:
        addi t1, s0, 320
        addi t2, s0, 352
        li t3, 2
        1:
        lw t0, 0(t1)
        lw t4, 0(t2)
        bne t0, t4, 5f
        addi t1, t1, 4
        addi t2, t2, 4
        addi t3, t3, -1
        bnez t3, 1b
        li t0, 1
        j 6f
        5:
        li t0, 0
        6:
    bool.147.12.end:
    func.assert.147.5:
        if.28.27.147.5:
        cmp.28.27.147.5:
        bne t0, zero, if.28.24.147.5.end
        if.28.27.147.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.147.5.end:
    func.assert.147.5.end:
    li t2, 8
    addi t0, s0, 84
    addi t1, s0, 52
    slli t2, t2, 2
    srli t4, t2, 2
    andi t2, t2, 3
    beqz t4, 2f
    1:
    lw t3, 0(t0)
    sw t3, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    addi t4, t4, -1
    bnez t4, 1b
    2:
    andi t4, t2, 2
    beqz t4, 3f
    lhu t3, 0(t0)
    sh t3, 0(t1)
    addi t0, t0, 2
    addi t1, t1, 2
    3:
    andi t2, t2, 1
    beqz t2, 4f
    lbu t3, 0(t0)
    sb t3, 0(t1)
    4:
    addi t0, s0, 312
    sw t0, 360(s0)
    addi s1, s0, 360
    call func.print_num
    cmp.159.12:
    lb t0, 329(s0)
    sltiu t0, t0, 1
    bool.159.12.end:
    func.assert.159.5:
        if.28.27.159.5:
        cmp.28.27.159.5:
        bne t0, zero, if.28.24.159.5.end
        if.28.27.159.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.159.5.end:
    func.assert.159.5.end:
    cmp.160.12:
    lb t0, 316(s0)
    sltiu t0, t0, 1
    bool.160.12.end:
    func.assert.160.5:
        if.28.27.160.5:
        cmp.28.27.160.5:
        bne t0, zero, if.28.24.160.5.end
        if.28.27.160.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.160.5.end:
    func.assert.160.5.end:
    cmp.161.12:
    lw t0, 320(s0)
    xori t0, t0, -1
    sltiu t0, t0, 1
    bool.161.12.end:
    func.assert.161.5:
        if.28.27.161.5:
        cmp.28.27.161.5:
        bne t0, zero, if.28.24.161.5.end
        if.28.27.161.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.161.5.end:
    func.assert.161.5.end:
    li a0, 0
    j .Lbaz_exit
func.print_num:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw zero, 4(s1)
    sw zero, 8(s1)
    sw zero, 12(s1)
    sw zero, 16(s1)
    sw zero, 20(s1)
    lw t0, 0(s1)
    lw t1, 0(t0)
    sw t1, 24(s1)
    li t0, 20
    sw t0, 28(s1)
    loop.44.5:
        lw t0, 28(s1)
        addi t0, t0, -1
        sw t0, 28(s1)
        lw t0, 28(s1)
            li t1, 48
            lw t2, 24(s1)
            addi a0, t2, 0
            li a1, 10
            call .Lbaz_divide
            addi t2, a1, 0
            sub t1, t1, t2
        add t2, s1, t0
        sb t1, 4(t2)
        lw a0, 24(s1)
        li a1, 10
        call .Lbaz_divide
        sw a0, 24(s1)
        if.55.12:
        cmp.55.12:
        lw t0, 24(s1)
        bne t0, zero, loop.44.5
        if.55.12.code:
        if.55.9.end:
    loop.44.5.end:
    li a0, 1
    li a2, 20
    addi a1, s1, 4
    call a7, .Lbaz_write
    lw ra, 0(sp)
    addi sp, sp, 16
    ret
.equ size.func.print_num, 32
.Lbaz_exit:
1:
    ebreak
    j 1b
.Lbaz_write:
    li a3, -1
    mv a5, a1
    add a0, a1, a2
1:
    beq a5, a0, 3f
2:
    lw a4, -8(zero)
    bne a4, a3, 2b
    lbu a4, 0(a5)
    sw a4, -8(zero)
    addi a5, a5, 1
    j 1b
3:
    mv a0, a2
    jr a7
.Lbaz_divide:
    beqz a1, 5f
    srai a4, a0, 31
    srai a3, a1, 31
    xor a0, a0, a4
    sub a0, a0, a4
    xor a1, a1, a3
    sub a1, a1, a3
    xor a3, a3, a4
    li a2, 0
    li a5, 32
1:
    srli a6, a0, 31
    slli a2, a2, 1
    or a2, a2, a6
    slli a0, a0, 1
    bltu a2, a1, 2f
    sub a2, a2, a1
    ori a0, a0, 1
2:
    addi a5, a5, -1
    bnez a5, 1b
    xor a0, a0, a3
    sub a0, a0, a3
    xor a1, a2, a4
    sub a1, a1, a4
    ret
5:
    ebreak
    j 5b
.data
.balign 16
dat:
dat.end:
.bss
.balign 16
vars:
.zero 65536
vars.end:
