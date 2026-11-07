.option norvc
.option norelax
.text
.globl _start
_start:
la s0, vars
addi s0, s0, 2032
main:
    li t0, 1
    sw t0, -2032(s0)
    sw zero, -2028(s0)
    sw zero, -2024(s0)
    sw zero, -2020(s0)
    sw zero, -2016(s0)
    li t0, 3
    sw t0, -2012(s0)
    li t0, 5
    sw t0, -2008(s0)
    sw zero, -2004(s0)
    sw zero, -2000(s0)
    sw zero, -1996(s0)
    sw zero, -1992(s0)
    sw zero, -1988(s0)
    sw zero, -1984(s0)
    addi t0, s0, -1980
    addi t1, t0, 256
    1:
    sw zero, 0(t0)
    addi t0, t0, 4
    bne t0, t1, 1b
    cmp.68.12:
        lw t1, -2012(s0)
        addi t1, t1, 2
    xori t0, t1, 5
    sltiu t0, t0, 1
    bool.68.12.end:
    func.assert..68.5:
        if.28.24.68.5:
        cmp.28.27.68.5:
        bne t0, zero, if.28.24.68.5.end
        if.28.24.68.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.68.5.end:
    func.assert..68.5.end:
    func.assert..69.5:
        if.28.24.69.5:
        cmp.28.27.69.5:
        if.28.24.69.5.end:
    func.assert..69.5.end:
    li t0, 2
    sw t0, -2024(s0)
    cmp.72.12:
    xori t0, t0, 2
    sltiu t0, t0, 1
    bool.72.12.end:
    func.assert..72.5:
        if.28.24.72.5:
        cmp.28.27.72.5:
        bne t0, zero, if.28.24.72.5.end
        if.28.24.72.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.72.5.end:
    func.assert..72.5.end:
    li t0, 74
    sw t0, -1988(s0)
    cmp.75.12:
    xori t0, t0, 74
    sltiu t0, t0, 1
    bool.75.12.end:
    func.assert..75.5:
        if.28.24.75.5:
        cmp.28.27.75.5:
        bne t0, zero, if.28.24.75.5.end
        if.28.24.75.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.75.5.end:
    func.assert..75.5.end:
    li t0, 65518
    sw t0, -1944(s0)
    li t0, 1
    sw t0, -1724(s0)
    lw t1, -1724(s0)
    slli t1, t1, 2
    add t1, t1, s0
    lw t1, -2028(t1)
    slli t2, t0, 2
    add t2, t2, s0
    sw t1, -2024(t2)
    lw t0, -1724(s0)
    slli t1, t0, 5
    add t1, t1, s0
    li t2, 65518
    sw t2, -1976(t1)
    lw t0, -2032(s0)
    sub t0, zero, t0
    sw t0, -1720(s0)
    lw t0, -2032(s0)
    xori t0, t0, -1
    sw t0, -1720(s0)
    sb zero, -1716(s0)
    lw t0, -2032(s0)
    sb t0, -1716(s0)
    lb t0, -1716(s0)
    addi t0, t0, -1
    sb t0, -1716(s0)
    lw t0, -2032(s0)
    sub t0, zero, t0
    sw t0, -1712(s0)
    lw t0, -1720(s0)
    sub t0, zero, t0
    sw t0, -1708(s0)
    lw t0, -1724(s0)
    func.inv..97.21:
        slli t1, t0, 2
        add t1, t1, s0
        lw t1, -2032(t1)
        xori t1, t1, -1
        sw t1, -1704(s0)
    func.inv..97.21.end:
    lw t0, -1704(s0)
    xori t0, t0, -1
    sw t0, -1704(s0)
    lw t0, -1724(s0)
    lw t1, -1704(s0)
    slli t2, t0, 2
    add t2, t2, s0
    sw t1, -2028(t2)
    cmp.102.12:
    lw t0, -2032(s0)
    xori t0, t0, 1
    sltiu t0, t0, 1
    bool.102.12.end:
    func.assert..102.5:
        if.28.24.102.5:
        cmp.28.27.102.5:
        bne t0, zero, if.28.24.102.5.end
        if.28.24.102.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.102.5.end:
    func.assert..102.5.end:
    cmp.104.14:
    lw t0, -2032(s0)
    xori t0, t0, 1
    sltiu t0, t0, 1
    sb t0, -1700(s0)
    bool.104.14.end:
    func.assert..105.5:
        if.28.24.105.5:
        cmp.28.27.105.5:
        lbu t0, -1700(s0)
        bne t0, zero, if.28.24.105.5.end
        if.28.24.105.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.105.5.end:
    func.assert..105.5.end:
    lw t0, -2032(s0)
    slli t1, t0, 2
    add t0, t1, t0
    slli t0, t0, 1
    sw t0, -1720(s0)
    lw t0, -1724(s0)
    slli t0, t0, 2
    add t0, t0, s0
    lw t0, -2028(t0)
    sw t0, -1720(s0)
    lw t0, -1724(s0)
    slli t1, t0, 2
    sub t0, t1, t0
    slli t0, t0, 2
    func.object.at..113.14:
        func.point.at..40.16.113.14:
            add t1, s0, t0
            li t2, 2
            sw t2, -2004(t1)
            add t1, s0, t0
            li t2, 74
            sw t2, -2000(t1)
        func.point.at..40.16.113.14.end:
        add t1, s0, t0
        li t2, 16777215
        sw t2, -1996(t1)
    func.object.at..113.14.end:
    sb zero, -1699(s0)
    sh zero, -1698(s0)
    sw zero, -1696(s0)
    sw zero, -1692(s0)
    sw zero, -1688(s0)
    sw zero, -1684(s0)
    sb zero, -1680(s0)
    lw t0, -1712(s0)
    sw t0, -1676(s0)
    lw t0, -1708(s0)
    sw t0, -1672(s0)
    cmp.118.12:
        lw t0, -1712(s0)
        lw t1, -1676(s0)
        bne t0, t1, 5f
        lw t0, -1708(s0)
        lw t1, -1672(s0)
        bne t0, t1, 5f
        li t0, 1
        j 6f
        5:
        li t0, 0
        6:
    bool.118.12.end:
    func.assert..118.5:
        if.28.24.118.5:
        cmp.28.27.118.5:
        bne t0, zero, if.28.24.118.5.end
        if.28.24.118.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.118.5.end:
    func.assert..118.5.end:
    addi t1, s0, -1948
    addi t2, s0, -1980
    addi t3, t1, 32
    1:
    lw t0, 0(t1)
    sw t0, 0(t2)
    addi t1, t1, 4
    addi t2, t2, 4
    bne t1, t3, 1b
    addi t0, s0, -1720
    sw t0, -1668(s0)
    addi s1, s0, -1668
    call func.print_num
    cmp.125.12:
    lb t0, -1699(s0)
    sltiu t0, t0, 1
    bool.125.12.end:
    func.assert..125.5:
        if.28.24.125.5:
        cmp.28.27.125.5:
        bne t0, zero, if.28.24.125.5.end
        if.28.24.125.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.125.5.end:
    func.assert..125.5.end:
    cmp.126.12:
    lb t0, -1716(s0)
    sltiu t0, t0, 1
    bool.126.12.end:
    func.assert..126.5:
        if.28.24.126.5:
        cmp.28.27.126.5:
        bne t0, zero, if.28.24.126.5.end
        if.28.24.126.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.126.5.end:
    func.assert..126.5.end:
    cmp.127.12:
    lw t0, -1712(s0)
    xori t0, t0, -1
    sltiu t0, t0, 1
    bool.127.12.end:
    func.assert..127.5:
        if.28.24.127.5:
        cmp.28.27.127.5:
        bne t0, zero, if.28.24.127.5.end
        if.28.24.127.5.code:
            li a0, 1
            j .Lbaz_exit
        if.28.24.127.5.end:
    func.assert..127.5.end:
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
    lw t0, 0(t0)
    sw t0, 24(s1)
    li t0, 20
    sw t0, 28(s1)
    loop.48.5:
        lw t0, 28(s1)
        addi t0, t0, -1
        sw t0, 28(s1)
        li t1, 48
        lw a0, 24(s1)
        li a1, 10
        call .Lbaz_divide
        sub t1, t1, a1
        add t2, s1, t0
        sb t1, 4(t2)
        lw a0, 24(s1)
        li a1, 10
        call .Lbaz_divide
        sw a0, 24(s1)
        if.56.9:
        cmp.56.12:
        lw t0, 24(s1)
        bne t0, zero, loop.48.5
        if.56.9.code:
        if.56.9.end:
    loop.48.5.end:
    li a0, 1
    addi a1, s1, 4
    li a2, 20
    li a7, 64
    ecall
    lw ra, 0(sp)
    addi sp, sp, 16
    ret
.equ size.func.print_num, 32
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
    li a0, 253
.Lbaz_exit:
    li a7, 93
    ecall
.data
.balign 16
dat:
dat.end:
.bss
.balign 16
vars:
.zero 65536
vars.end:
