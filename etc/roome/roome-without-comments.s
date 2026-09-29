.option norvc
.option norelax
.text
.globl _start
_start:
la s0, dat
lui sp, 2048
main:
    func.print.195.5:
        li a0, 1
        li a2, 41
        addi a1, s0, 0
        call a7, .Lbaz_write
    func.print.195.5.end:
    lui t0, 37
    add t0, t0, s0
    addi t0, t0, -1552
    li t1, 32
    1:
    sw zero, 0(t0)
    addi t0, t0, 4
    addi t1, t1, -1
    bnez t1, 1b
    lui t0, 37
    add t0, t0, s0
    sw zero, -1424(t0)
    loop.199.5:
        if.200.12:
        cmp.200.12:
        lui t0, 37
        add t0, t0, s0
        lw t0, -1424(t0)
        lui t1, 37
        add t1, t1, s0
        lw t1, -1588(t1)
        bne t0, t1, if.200.9.end
        if.200.12.code:
            lui t0, 37
            add t0, t0, s0
            sw zero, -1424(t0)
        if.200.9.end:
        func.print.203.9:
            li a0, 1
            li a2, 1
            addi a1, s0, 55
            call a7, .Lbaz_write
        func.print.203.9.end:
        lui t1, 37
        add t1, t1, s0
        lw t1, -1424(t1)
        bltz t1, 1f
        li t2, 32
        bltu t1, t2, 2f
        1:
        li a0, 204
        j baz_bounds_panic
        2:
        slli t2, t1, 7
        add t2, t2, t1
        slli t2, t2, 2
        add t1, t2, t1
        slli t1, t1, 2
        add t0, s0, t1
        lui t2, 20
        add t0, t0, t2
        lh t0, 1886(t0)
        bltz t0, 1f
        li t1, 128
        bltu t0, t1, 2f
        1:
        li a0, 204
        j baz_bounds_panic
        2:
        slli t1, t0, 6
        add t1, t1, t0
        slli t1, t1, 2
        sub t0, t1, t0
        slli t0, t0, 1
        add t1, s0, t0
        lui t2, 4
        add t1, t1, t2
        addi t1, t1, 1100
        func.room.print.204.45:
            func.str.print.116.22.204.45:
                li a0, 1
                lb a2, 144(t1)
                bltz a2, 1f
                li t2, 127
                bgeu t2, a2, 2f
                1:
                li a0, 23
                j baz_bounds_panic
                2:
                addi a1, t1, 17
                call a7, .Lbaz_write
            func.str.print.116.22.204.45.end:
            func.print.117.5.204.45:
                li a0, 1
                li a2, 1
                addi a1, s0, 55
                call a7, .Lbaz_write
            func.print.117.5.204.45.end:
            if.118.7.204.45:
            cmp.118.7.204.45:
            cmp.118.8.204.45:
            lh t2, 338(t1)
            bge zero, t2, if.118.5.204.45.end
            if.118.7.204.45.code:
                func.print.119.9.204.45:
                    li a0, 1
                    li a2, 4
                    addi a1, s0, 41
                    call a7, .Lbaz_write
                func.print.119.9.204.45.end:
                lui t2, 37
                add t2, t2, s0
                sh zero, -1420(t2)
                loop.121.9.204.45:
                    if.122.16.204.45:
                    cmp.122.16.204.45:
                    lh t2, 338(t1)
                    lui t3, 37
                    add t3, t3, s0
                    lh t3, -1420(t3)
                    beq t2, t3, loop.121.9.204.45.end
                    if.122.16.204.45.code:
                    if.122.13.204.45.end:
                    lui t3, 37
                    add t3, t3, s0
                    lh t3, -1420(t3)
                    bltz t3, 1f
                    li t4, 32
                    bltu t3, t4, 2f
                    1:
                    li a0, 123
                    j baz_bounds_panic
                    2:
                    slli t2, t3, 1
                    add t2, t2, t1
                    lh t2, 274(t2)
                    bltz t2, 1f
                    li t3, 32
                    bltu t2, t3, 2f
                    1:
                    li a0, 123
                    j baz_bounds_panic
                    2:
                    slli t3, t2, 7
                    add t3, t3, t2
                    slli t3, t3, 2
                    add t2, t3, t2
                    slli t2, t2, 2
                    add t3, s0, t2
                    lui t4, 20
                    add t3, t3, t4
                    addi t3, t3, 1868
                    func.entity.print.123.44.204.45:
                        li a0, 1
                        lb a2, 16(t3)
                        bltz a2, 1f
                        li t4, 16
                        bgeu t4, a2, 2f
                        1:
                        li a0, 96
                        j baz_bounds_panic
                        2:
                        addi a1, t3, 0
                        call a7, .Lbaz_write
                    func.entity.print.123.44.204.45.end:
                    func.print.124.13.204.45:
                        li a0, 1
                        li a2, 1
                        addi a1, s0, 55
                        call a7, .Lbaz_write
                    func.print.124.13.204.45.end:
                    lui t2, 37
                    add t2, t2, s0
                    lh t3, -1420(t2)
                    addi t3, t3, 1
                    sh t3, -1420(t2)
                j loop.121.9.204.45
                loop.121.9.204.45.end:
            if.118.5.204.45.end:
            if.128.7.204.45:
            cmp.128.7.204.45:
            cmp.128.8.204.45:
            lb t2, 388(t1)
            bge zero, t2, if.128.5.204.45.end
            if.128.7.204.45.code:
                func.print.129.9.204.45:
                    li a0, 1
                    li a2, 7
                    addi a1, s0, 45
                    call a7, .Lbaz_write
                func.print.129.9.204.45.end:
                lui t2, 37
                add t2, t2, s0
                sb zero, -1420(t2)
                loop.131.9.204.45:
                    if.132.16.204.45:
                    cmp.132.16.204.45:
                    lb t2, 388(t1)
                    lui t3, 37
                    add t3, t3, s0
                    lb t3, -1420(t3)
                    beq t2, t3, loop.131.9.204.45.end
                    if.132.16.204.45.code:
                    if.132.13.204.45.end:
                    lui t3, 37
                    add t3, t3, s0
                    lb t3, -1420(t3)
                    bltz t3, 1f
                    li t4, 8
                    bltu t3, t4, 2f
                    1:
                    li a0, 133
                    j baz_bounds_panic
                    2:
                    slli t4, t3, 2
                    sub t3, t4, t3
                    slli t3, t3, 1
                    add t2, t1, t3
                    lh t2, 340(t2)
                    bltz t2, 1f
                    li t3, 1024
                    bltu t2, t3, 2f
                    1:
                    li a0, 133
                    j baz_bounds_panic
                    2:
                    slli t3, t2, 4
                    add t2, t3, t2
                    add t3, s0, t2
                    func.name.print.133.52.204.45:
                        li a0, 1
                        lb a2, 92(t3)
                        bltz a2, 1f
                        li t4, 16
                        bgeu t4, a2, 2f
                        1:
                        li a0, 66
                        j baz_bounds_panic
                        2:
                        addi a1, t3, 76
                        call a7, .Lbaz_write
                    func.name.print.133.52.204.45.end:
                    func.print.134.13.204.45:
                        li a0, 1
                        li a2, 1
                        addi a1, s0, 55
                        call a7, .Lbaz_write
                    func.print.134.13.204.45.end:
                    lui t2, 37
                    add t2, t2, s0
                    lb t3, -1420(t2)
                    addi t3, t3, 1
                    sb t3, -1420(t2)
                j loop.131.9.204.45
                loop.131.9.204.45.end:
            if.128.5.204.45.end:
            if.138.8.204.45:
            cmp.138.8.204.45:
            lb t2, 272(t1)
            beq t2, zero, if.138.5.204.45.end
            if.138.8.204.45.code:
                func.str.print.139.19.204.45:
                    li a0, 1
                    lb a2, 272(t1)
                    bltz a2, 1f
                    li t2, 127
                    bgeu t2, a2, 2f
                    1:
                    li a0, 23
                    j baz_bounds_panic
                    2:
                    addi a1, t1, 145
                    call a7, .Lbaz_write
                func.str.print.139.19.204.45.end:
                func.print.140.9.204.45:
                    li a0, 1
                    li a2, 1
                    addi a1, s0, 55
                    call a7, .Lbaz_write
                func.print.140.9.204.45.end:
            if.138.5.204.45.end:
        func.room.print.204.45.end:
        lui t0, 37
        add t0, t0, s0
        lw t0, -1424(t0)
        bltz t0, 1f
        li t1, 32
        bltu t0, t1, 2f
        1:
        li a0, 205
        j baz_bounds_panic
        2:
        slli t1, t0, 7
        add t1, t1, t0
        slli t1, t1, 2
        add t0, t1, t0
        slli t0, t0, 2
        add t1, s0, t0
        lui t2, 20
        add t1, t1, t2
        addi t1, t1, 1868
        func.name.print.205.35:
            li a0, 1
            lb a2, 16(t1)
            bltz a2, 1f
            li t2, 16
            bgeu t2, a2, 2f
            1:
            li a0, 66
            j baz_bounds_panic
            2:
            addi a1, t1, 0
            call a7, .Lbaz_write
        func.name.print.205.35.end:
        func.print.206.9:
            li a0, 1
            li a2, 3
            addi a1, s0, 52
            call a7, .Lbaz_write
        func.print.206.9.end:
        func.str.input.207.13:
            lui t0, 37
            add t0, t0, s0
            sw zero, -1420(t0)
            loop.29.5.207.13:
                if.30.12.207.13:
                cmp.30.12.207.13:
                    li t0, 127
                lui t1, 37
                add t1, t1, s0
                lw t1, -1420(t1)
                beq t1, t0, loop.29.5.207.13.end
                if.30.12.207.13.code:
                if.30.9.207.13.end:
                if.31.12.207.13:
                cmp.31.12.207.13:
                    li a0, 0
                    li a2, 1
                    lui t1, 37
                    add t1, t1, s0
                    lw t1, -1420(t1)
                    bltz t1, 1f
                    bltz a2, 1f
                    add t3, t1, a2
                    li t2, 127
                    bgeu t2, t3, 2f
                    1:
                    li a0, 31
                    j baz_bounds_panic
                    2:
                    lui a1, 37
                    add a1, a1, s0
                    addi a1, a1, -1552
                    add a1, a1, t1
                    call a7, .Lbaz_read
                    addi t0, a0, 0
                beq t0, zero, loop.29.5.207.13.end
                if.31.12.207.13.code:
                if.31.9.207.13.end:
                if.32.12.207.13:
                cmp.32.12.207.13:
                lui t0, 37
                add t0, t0, s0
                lw t0, -1420(t0)
                bltz t0, 1f
                li t1, 127
                bltu t0, t1, 2f
                1:
                li a0, 32
                j baz_bounds_panic
                2:
                add t1, s0, t0
                lui t2, 37
                add t1, t1, t2
                lb t1, -1552(t1)
                li t2, 127
                bne t1, t2, if.32.9.207.13.end
                if.32.12.207.13.code:
                    if.33.16.207.13:
                    cmp.33.16.207.13:
                    lui t0, 37
                    add t0, t0, s0
                    lw t0, -1420(t0)
                    bge zero, t0, if.33.13.207.13.end
                    if.33.16.207.13.code:
                        lui t0, 37
                        add t0, t0, s0
                        lw t1, -1420(t0)
                        addi t1, t1, -1
                        sw t1, -1420(t0)
                        li a0, 1
                        li a2, 3
                        addi a1, s0, 56
                        call a7, .Lbaz_write
                    if.33.13.207.13.end:
                    j loop.29.5.207.13
                if.32.9.207.13.end:
                li a0, 1
                li a2, 1
                lui t0, 37
                add t0, t0, s0
                lw t0, -1420(t0)
                bltz t0, 1f
                bltz a2, 1f
                add t2, t0, a2
                li t1, 127
                bgeu t1, t2, 2f
                1:
                li a0, 39
                j baz_bounds_panic
                2:
                lui a1, 37
                add a1, a1, s0
                addi a1, a1, -1552
                add a1, a1, t0
                call a7, .Lbaz_write
                if.40.12.207.13:
                cmp.40.12.207.13:
                lui t0, 37
                add t0, t0, s0
                lw t0, -1420(t0)
                bltz t0, 1f
                li t1, 127
                bltu t0, t1, 2f
                1:
                li a0, 40
                j baz_bounds_panic
                2:
                add t1, s0, t0
                lui t2, 37
                add t1, t1, t2
                lb t1, -1552(t1)
                li t2, 10
                beq t1, t2, loop.29.5.207.13.end
                if.40.12.207.13.code:
                if.40.9.207.13.end:
                lui t0, 37
                add t0, t0, s0
                lw t1, -1420(t0)
                addi t1, t1, 1
                sw t1, -1420(t0)
            j loop.29.5.207.13
            loop.29.5.207.13.end:
            lui t0, 37
            add t0, t0, s0
            lw t0, -1420(t0)
            lui t1, 37
            add t1, t1, s0
            sb t0, -1425(t1)
        func.str.input.207.13.end:
        func.parse_input.208.9:
            if.184.8.208.9:
            cmp.184.8.208.9:
                lui t0, 37
                add t0, t0, s0
                lb t0, -1425(t0)
                li t1, 2
            blt t0, t1, if.184.5.208.9.end
            cmp.185.8.208.9:
                li t3, 2
                bltz t3, 1f
                li t4, 2
                bgeu t4, t3, 2f
                1:
                li a0, 185
                j baz_bounds_panic
                2:
                addi t1, s0, 74
                bltz t3, 1f
                li t4, 127
                bgeu t4, t3, 2f
                1:
                li a0, 185
                j baz_bounds_panic
                2:
                lui t2, 37
                add t2, t2, s0
                addi t2, t2, -1552
                srli t5, t3, 1
                andi t3, t3, 1
                beqz t5, 2f
                1:
                lhu t0, 0(t1)
                lhu t4, 0(t2)
                bne t0, t4, 5f
                addi t1, t1, 2
                addi t2, t2, 2
                addi t5, t5, -1
                bnez t5, 1b
                2:
                beqz t3, 4f
                lbu t0, 0(t1)
                lbu t4, 0(t2)
                bne t0, t4, 5f
                4:
                li t0, 1
                j 6f
                5:
                li t0, 0
                6:
            beq t0, zero, if.184.5.208.9.end
            if.184.8.208.9.code:
                li t0, 2
                func.action_go.186.9.208.9:
                    lui t1, 37
                    add t1, t1, s0
                    sw t0, -1420(t1)
                    lui t1, 37
                    add t1, t1, s0
                    lw t1, -1420(t1)
                    lui t2, 37
                    add t2, t2, s0
                    sw t1, -1416(t2)
                    func.next_token.169.5.186.9.208.9:
                        loop.149.5.169.5.186.9.208.9:
                            if.150.12.169.5.186.9.208.9:
                            cmp.150.12.169.5.186.9.208.9:
                            lui t1, 37
                            add t1, t1, s0
                            lw t1, -1420(t1)
                            lui t2, 37
                            add t2, t2, s0
                            lb t2, -1425(t2)
                            blt t1, t2, if.150.9.169.5.186.9.208.9.end
                            if.150.12.169.5.186.9.208.9.code:
                                lui t1, 37
                                add t1, t1, s0
                                lw t1, -1420(t1)
                                lui t2, 37
                                add t2, t2, s0
                                sw t1, -1416(t2)
                                j func.next_token.169.5.186.9.208.9.end
                            if.150.9.169.5.186.9.208.9.end:
                            if.154.12.169.5.186.9.208.9:
                            cmp.154.12.169.5.186.9.208.9:
                            lui t1, 37
                            add t1, t1, s0
                            lw t1, -1420(t1)
                            bltz t1, 1f
                            li t2, 127
                            bltu t1, t2, 2f
                            1:
                            li a0, 154
                            j baz_bounds_panic
                            2:
                            add t2, s0, t1
                            lui t3, 37
                            add t2, t2, t3
                            lb t2, -1552(t2)
                            li t3, 32
                            bne t2, t3, loop.149.5.169.5.186.9.208.9.end
                            if.154.12.169.5.186.9.208.9.code:
                            if.154.9.169.5.186.9.208.9.end:
                            lui t1, 37
                            add t1, t1, s0
                            lw t2, -1420(t1)
                            addi t2, t2, 1
                            sw t2, -1420(t1)
                        j loop.149.5.169.5.186.9.208.9
                        loop.149.5.169.5.186.9.208.9.end:
                        lui t1, 37
                        add t1, t1, s0
                        lw t1, -1420(t1)
                        lui t2, 37
                        add t2, t2, s0
                        sw t1, -1416(t2)
                        loop.159.5.169.5.186.9.208.9:
                            if.160.12.169.5.186.9.208.9:
                            cmp.160.12.169.5.186.9.208.9:
                            lui t1, 37
                            add t1, t1, s0
                            lw t1, -1416(t1)
                            lui t2, 37
                            add t2, t2, s0
                            lb t2, -1425(t2)
                            bge t1, t2, loop.159.5.169.5.186.9.208.9.end
                            if.160.12.169.5.186.9.208.9.code:
                            if.160.9.169.5.186.9.208.9.end:
                            if.161.12.169.5.186.9.208.9:
                            cmp.161.12.169.5.186.9.208.9:
                            lui t1, 37
                            add t1, t1, s0
                            lw t1, -1416(t1)
                            bltz t1, 1f
                            li t2, 127
                            bltu t1, t2, 2f
                            1:
                            li a0, 161
                            j baz_bounds_panic
                            2:
                            add t2, s0, t1
                            lui t3, 37
                            add t2, t2, t3
                            lb t2, -1552(t2)
                            li t3, 32
                            beq t2, t3, loop.159.5.169.5.186.9.208.9.end
                            if.161.12.169.5.186.9.208.9.code:
                            if.161.9.169.5.186.9.208.9.end:
                            lui t1, 37
                            add t1, t1, s0
                            lw t2, -1416(t1)
                            addi t2, t2, 1
                            sw t2, -1416(t1)
                        j loop.159.5.169.5.186.9.208.9
                        loop.159.5.169.5.186.9.208.9.end:
                    func.next_token.169.5.186.9.208.9.end:
                    if.170.8.186.9.208.9:
                    cmp.170.8.186.9.208.9:
                    lui t1, 37
                    add t1, t1, s0
                    lw t1, -1420(t1)
                    lui t2, 37
                    add t2, t2, s0
                    lw t2, -1416(t2)
                    bne t1, t2, if.170.5.186.9.208.9.end
                    if.170.8.186.9.208.9.code:
                        func.print.171.9.186.9.208.9:
                            li a0, 1
                            li a2, 9
                            lui a1, 37
                            add a1, a1, s0
                            addi a1, a1, -1576
                            call a7, .Lbaz_write
                        func.print.171.9.186.9.208.9.end:
                        j func.action_go.186.9.208.9.end
                    if.170.5.186.9.208.9.end:
                    lui t1, 37
                    add t1, t1, s0
                    addi t1, t1, -1412
                    li t2, 32
                    1:
                    sw zero, 0(t1)
                    addi t1, t1, 4
                    addi t2, t2, -1
                    bnez t2, 1b
                    lui t1, 37
                    add t1, t1, s0
                    lw t1, -1416(t1)
                    lui t2, 37
                    add t2, t2, s0
                    lw t2, -1420(t2)
                    sub t1, t1, t2
                    lui t2, 37
                    add t2, t2, s0
                    sb t1, -1285(t2)
                    lui t3, 37
                    add t3, t3, s0
                    lb t3, -1285(t3)
                    addi t3, t3, 200
                    lui t4, 37
                    add t4, t4, s0
                    lw t4, -1420(t4)
                    bltz t4, 1f
                    bltz t3, 1f
                    add t6, t4, t3
                    li t5, 127
                    bgeu t5, t6, 2f
                    1:
                    li a0, 177
                    j baz_bounds_panic
                    2:
                    add t1, s0, t4
                    lui t5, 37
                    add t1, t1, t5
                    addi t1, t1, -1552
                    bltz t3, 1f
                    li t4, 127
                    bgeu t4, t3, 2f
                    1:
                    li a0, 177
                    j baz_bounds_panic
                    2:
                    lui t2, 37
                    add t2, t2, s0
                    addi t2, t2, -1412
                    beqz t3, 4f
                    1:
                    lbu t4, 0(t1)
                    sb t4, 0(t2)
                    addi t1, t1, 1
                    addi t2, t2, 1
                    addi t3, t3, -1
                    bnez t3, 1b
                    4:
                    func.print.178.5.186.9.208.9:
                        li a0, 1
                        li a2, 8
                        lui a1, 37
                        add a1, a1, s0
                        addi a1, a1, -1584
                        call a7, .Lbaz_write
                    func.print.178.5.186.9.208.9.end:
                    func.str.print.179.8.186.9.208.9:
                        li a0, 1
                        lui a2, 37
                        add a2, a2, s0
                        lb a2, -1285(a2)
                        bltz a2, 1f
                        li t1, 127
                        bgeu t1, a2, 2f
                        1:
                        li a0, 23
                        j baz_bounds_panic
                        2:
                        lui a1, 37
                        add a1, a1, s0
                        addi a1, a1, -1412
                        call a7, .Lbaz_write
                    func.str.print.179.8.186.9.208.9.end:
                    func.print.180.5.186.9.208.9:
                        li a0, 1
                        li a2, 1
                        addi a1, s0, 55
                        call a7, .Lbaz_write
                    func.print.180.5.186.9.208.9.end:
                func.action_go.186.9.208.9.end:
                j func.parse_input.208.9.end
            if.184.5.208.9.end:
            func.print.190.5.208.9:
                li a0, 1
                li a2, 14
                addi a1, s0, 59
                call a7, .Lbaz_write
            func.print.190.5.208.9.end:
            func.print.191.5.208.9:
                li a0, 1
                li a2, 1
                addi a1, s0, 55
                call a7, .Lbaz_write
            func.print.191.5.208.9.end:
        func.parse_input.208.9.end:
    j loop.199.5
    loop.199.5.end:
baz_frame_overflow:
    li a0, 2
    la a1, .Lbaz_frame_message
    li a2, 22
    call a7, .Lbaz_write
    li a0, 255
    j .Lbaz_exit
.section .rodata
.Lbaz_frame_message:
.ascii "panic: frame overflow"
.byte 10
.text
baz_bounds_panic:
    mv s2, a0
    li a0, 2
    la a1, .Lbaz_bounds_message
    li a2, 22
    call a7, .Lbaz_write
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
    call a7, .Lbaz_write
    li a0, 255
.section .rodata
.Lbaz_bounds_message:
.ascii "panic: bounds at line "
.balign 4
.Lbaz_decimal_places:
.word 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1
.text
.Lbaz_exit:
1:
    ebreak
    j 1b
.Lbaz_read:
    li a3, -1
    mv a6, a1
    li a0, 0
1:
    beq a0, a2, 4f
2:
    lw a4, -12(zero)
    beq a4, a3, 2b
    li a5, 4
    beq a4, a5, 4f
    li a5, 13
    bne a4, a5, 3f
    li a4, 10
3:
    sb a4, 0(a6)
    addi a6, a6, 1
    addi a0, a0, 1
    li a5, 10
    bne a4, a5, 1b
4:
    jr a7
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
.data
.balign 16
dat:
.ascii "\nwelcome to adventure #6\n    type 'help'\n"
.ascii "u c "
.ascii "exits: "
.ascii " > "
.ascii "\n"
.ascii "\010 \010"
.ascii "not understood"
.ascii "l"
.ascii "go"
.ascii "none"
.rept 12
.byte 0
.endr
.rept 1
.byte 4
.endr
.zero 17391
.ascii "roome"
.rept 11
.byte 0
.endr
.rept 1
.byte 5
.endr
.ascii "u r in roome"
.rept 115
.byte 0
.endr
.rept 1
.byte 12
.endr
.ascii "todo: find an exit"
.rept 109
.byte 0
.endr
.rept 1
.byte 18
.endr
.zero 1
.rept 1
.half 0
.endr
.rept 31
.half 0
.endr
.rept 1
.half 1
.endr
.rept 1
.half 0
.endr
.rept 1
.half 0
.endr
.rept 1
.half 0
.endr
.zero 42
.rept 1
.byte 1
.endr
.zero 129
.zero 65786
.ascii "me"
.rept 14
.byte 0
.endr
.rept 1
.byte 2
.endr
.zero 1
.rept 1
.half 0
.endr
.zero 2048
.zero 64108
.rept 1
.word 1
.endr
.ascii "went to "
.ascii "go where\n"
dat.end:
.bss
.balign 16
vars:
.zero 65536
vars.end:
