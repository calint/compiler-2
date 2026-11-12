// aligned accesses of each kind that must not trap, then the one misaligned
// access, odd jump, unsupported encoding or access between the device
// registers selected by 'CASE'; without a trap the program exits with 0

.global _start

_start:
	addi x10, x0, 0			# a0 = 0, the exit code without a trap
	lui x5, 0x1				# x5 = 0x0000_1000
	sw x0, 0(x5)
	lw x6, 0(x5)
	sh x0, 2(x5)
	lh x6, 2(x5)
	lhu x6, 2(x5)
	sb x0, 3(x5)
	lb x6, 3(x5)
	lbu x6, 3(x5)
#if CASE == 1
	jalr x0, %lo(done + 2)(x0)	# fetch at 0x2e, without the trap 'ebreak' at 0x2c
#elif CASE == 2
	lh x6, 1(x5)
#elif CASE == 3
	lhu x6, 3(x5)
#elif CASE == 4
	lw x6, 2(x5)
#elif CASE == 5
	sh x6, 1(x5)
#elif CASE == 6
	sw x6, 2(x5)
#elif CASE == 7
	jalr x0, %lo(done + 1)(x0)	# an odd target traps at the fetch
#elif CASE == 8
	.word 0x0252c333		# 'div x6, x5, x5' (M), not run as 'xor'
#elif CASE == 9
	.word 0x02029313		# 'slli x6, x5, 0' with 'funct7' 1
#elif CASE == 10
	.word 0x00029067		# 'jalr x0, 0(x5)' with 'funct3' 1
#elif CASE == 11
	.word 0x0000200f		# 'fence' with 'funct3' 2
#elif CASE == 12
	lbu x6, -11(x0)			# 0xffff'fff5, a device address but no register
#elif CASE == 13
	sb x0, -31(x0)			# 0xffff'ffe1, a device address but no register
#else
#error "CASE is 1 to 13"
#endif
done:
	ebreak
