#!/bin/bash
set -e
cd "$(dirname "$0")"

TARGET=x86_64
for arg in "$@"; do
	case "$arg" in
		--target=*) TARGET="${arg#--target=}" ;;
	esac
done
case "$TARGET" in
	x86_64|rv32i|rv32i-qemu) ;;
	*)
		printf 'Unsupported target: %s (use --target=x86_64, --target=rv32i or --target=rv32i-qemu)\n' "$TARGET" >&2
		exit 1
		;;
esac

# rv32i-qemu emits rv32i assembly
RV32I=
if [[ "$TARGET" == rv32i* ]]; then
	RV32I=1
fi

SEP="--------------------------------------------------------------------------------"
echo $SEP
printf './baz'
printf ' %q' "$@"
printf '\n'
./baz "$@" >gen.s
echo $SEP
COMMENT=';'
if [[ -n "$RV32I" ]]; then
	COMMENT='#'
fi
awk -v comment="$COMMENT" '$0 !~ "^[[:space:]]*" comment && $0 !~ /^[[:space:]]*$/ { print }' gen.s >gen-without-comments.s
if [[ -n "$RV32I" ]]; then
	awk '
		/^[[:space:]]*[[:alpha:]][[:alnum:]]*[[:space:]]/ {
			instructions++
			if ($1 == "j") jumps++
			if ($1 ~ /^b/) branches++
		}
		END { print "instructions: " instructions+0; print "jmp: " jumps+0; print "jcc: " branches+0 }
	' gen-without-comments.s
else
	grep -v -e'^\s*$' -e'^.*:\s*$' gen-without-comments.s | wc | awk '{print "instructions: " $1}'
	grep -E '^\s*jmp\s.*.*$' gen.s | wc | awk '{print "jmp: " $1}'
	grep -E '^\s*j[a-z]{1,2}\s' gen.s | grep -v '^\s*jmp\s' | wc | awk '{print "jcc: " $1}'
fi
echo $SEP
if [[ "$TARGET" == rv32i-qemu ]]; then
	# the compiler writes the image, no linking needed
	ls --color -la gen.s gen-without-comments.s gen-rv32i.bin
elif [[ "$TARGET" == rv32i ]]; then
	llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj gen.s -o gen.o
	ld.lld -m elf32lriscv -e _start -o gen gen.o
	ls --color -la gen.s gen-without-comments.s gen
else
	nasm -f elf64 gen.s
	ld -s -T baz.ld -o gen gen.o
	ls --color -la gen.s gen-without-comments.s gen
fi
echo $SEP

set +e # don't stop at errors
if [[ "$TARGET" == rv32i-qemu ]]; then
	./run-rv32i-qemu.sh gen-rv32i.bin
elif [[ "$TARGET" == rv32i ]]; then
	qemu-riscv32 ./gen
else
	./gen
fi
RET=$?
echo $SEP
echo returned: $RET
