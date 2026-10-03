#!/bin/bash
# checks the rv32i jumps that grow with the code between them: programs whose
# 'foo', 'if', 'break' and 'continue' jump over bodies of 8.4 KiB (a branch
# becomes 'j') and 1.08 MiB (a jump becomes 'jump'), compiled with and without
# the jump optimizer, assembled, linked and executed under qemu
#
# usage: test-far-jumps.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"

for tool in llvm-mc ld.lld qemu-riscv32 python3; do
    command -v "$tool" >/dev/null || {
        echo "Required tool not found: $tool" >&2
        exit 1
    }
done

export LLVM_PROFILE_FILE="$SCRIPT_DIR/far-jumps-%p.profraw"

# the compiler writes an image into the working directory
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# writes 'far.baz': each 'pad = pad + 1' is 12 bytes, so 700 needs 'j' and
# 90000 needs 'jump' for the jumps of 'foo', 'if', 'break' and 'continue'
generate_foo() {
    python3 - <<'EOF'
source = [
    "dat values = []{1, 2, 3, 4, 5}",
    "func main() {",
    "    var pad = 0",
    "    var visits = 0",
    "    var sum = 0",
]
for count in (700, 90000):
    padding = "        pad = pad + 1\n" * count
    # 'continue' at 2 runs the padding and 'break' at 4 skips it, so it runs
    # for 1, 2 and 3
    source.append(f"""    pad = 0
    visits = 0
    sum = 0
    foo values {{
        visits = visits + 1
        if e == 2 {{
{padding}            continue
        }}
        if e == 4 {{
            break
        }}
{padding}        sum = sum + e
    }}
    if visits != 4 exit(1)
    if sum != 4 exit(2)
    if pad != {3 * count} exit(3)""")
source.append("}")
open("far.baz", "w").write("\n".join(source) + "\n")
EOF
}

assemble_and_run() {
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj far.s -o far.o
    ld.lld -m elf32lriscv -e _start -o far far.o
    timeout -k 1s 60s qemu-riscv32 ./far
}

generate_foo
for optimize in "" "--nopt"; do
    echo -n "far jumps foo rv32i $optimize: "
    "$BIN" far.baz --target=rv32i --bin=far.bin $optimize >far.s
    grep -qE '^ +j foo\.[0-9]+\.[0-9]+$' far.s
    grep -qE '^ +jump foo\.[0-9]+\.[0-9]+, ' far.s
    grep -qE '^ +jump foo\.[0-9]+\.[0-9]+\.continue, ' far.s
    grep -qE '^ +jump foo\.[0-9]+\.[0-9]+\.end, ' far.s
    grep -qE '^ +j if\.[0-9]+\.[0-9]+\.end$' far.s
    grep -qE '^ +jump if\.[0-9]+\.[0-9]+\.end, ' far.s
    # a far conditional branch jumps over the far jump with the inverse branch
    grep -qE '^ +b[a-z]+ .*, \.Lbaz_jump\.[0-9]+$' far.s
    assemble_and_run
    echo ok
done
