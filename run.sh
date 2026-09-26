#!/bin/bash
set -e
cd "$(dirname "$0")"

TARGET=x86_64
# same default source as the compiler
SRC=prog.baz
for arg in "$@"; do
    case "$arg" in
    --target=*) TARGET="${arg#--target=}" ;;
    -*) ;;
    *) SRC="$arg" ;;
    esac
done
# keeps a directory part so the binary can be run as a path
BIN="$(dirname "$SRC")/$(basename "$SRC" .baz)"
ASM="$BIN.s"
ASM_NO_COMMENTS="$BIN-without-comments.s"
OBJ="$BIN.o"
case "$TARGET" in
x86_64 | rv32i | rv32i-qemu | rv32i-fpga) ;;
*)
    printf 'Unsupported target: %s (use --target=x86_64, --target=rv32i, --target=rv32i-qemu or --target=rv32i-fpga)\n' "$TARGET" >&2
    exit 1
    ;;
esac

# rv32i-qemu and rv32i-fpga emit rv32i assembly
RV32I=
if [[ "$TARGET" == rv32i* ]]; then
    RV32I=1
fi

SEP="--------------------------------------------------------------------------------"
echo $SEP
printf './baz'
printf ' %q' "$@"
printf '\n'
./baz "$@" >"$ASM"
echo $SEP
COMMENT=';'
if [[ -n "$RV32I" ]]; then
    COMMENT='#'
fi
awk -v comment="$COMMENT" '$0 !~ "^[[:space:]]*" comment && $0 !~ /^[[:space:]]*$/ { print }' "$ASM" >"$ASM_NO_COMMENTS"
if [[ -n "$RV32I" ]]; then
    awk '
		/^[[:space:]]*[[:alpha:]][[:alnum:]]*[[:space:]]/ {
			instructions++
			if ($1 == "j") jumps++
			if ($1 ~ /^b/) branches++
		}
		END { print "instructions: " instructions+0; print "jmp: " jumps+0; print "jcc: " branches+0 }
	' "$ASM_NO_COMMENTS"
else
    grep -v -e'^\s*$' -e'^.*:\s*$' "$ASM_NO_COMMENTS" | wc | awk '{print "instructions: " $1}'
    grep -E '^\s*jmp\s.*.*$' "$ASM" | wc | awk '{print "jmp: " $1}'
    grep -E '^\s*j[a-z]{1,2}\s' "$ASM" | grep -v '^\s*jmp\s' | wc | awk '{print "jcc: " $1}'
fi
echo $SEP
if [[ "$TARGET" == rv32i-qemu || "$TARGET" == rv32i-fpga ]]; then
    # the compiler writes the image, no linking needed
    ls --color -la "$ASM" "$ASM_NO_COMMENTS" gen-rv32i.bin
elif [[ "$TARGET" == rv32i ]]; then
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj "$ASM" -o "$OBJ"
    ld.lld -m elf32lriscv -e _start -o "$BIN" "$OBJ"
    ls --color -la "$ASM" "$ASM_NO_COMMENTS" "$BIN"
else
    nasm -f elf64 "$ASM"
    ld -s -T baz.ld -o "$BIN" "$OBJ"
    ls --color -la "$ASM" "$ASM_NO_COMMENTS" "$BIN"
fi
echo $SEP

set +e # don't stop at errors
if [[ "$TARGET" == rv32i-qemu ]]; then
    ./run-rv32i-qemu.sh gen-rv32i.bin
elif [[ "$TARGET" == rv32i ]]; then
    qemu-riscv32 "$BIN"
elif [[ "$TARGET" == rv32i-fpga ]]; then
    ./run-rv32i-fpga.sh gen-rv32i.bin
else
    "$BIN"
fi
RET=$?
echo $SEP
echo returned: $RET
