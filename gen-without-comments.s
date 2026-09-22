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
