#!/bin/bash
# checks the diagnostic of an rv32i jump beyond 1 MiB when no scratch register
# is free for its far form: a loop nested 14 deep with a body of 1.08 MiB;
# and that a branch the jump optimizer inverts takes the scratch register of
# the jump it replaces: a 'break' out of a 1.08 MiB loop body 13 'foo' deep,
# compiled with and without the jump optimizer and executed under qemu
#
# the body takes minutes to compile with the coverage build, so the check is a
# script of its own
#
# usage: test-far-jumps-no-scratch.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"

for tool in llvm-mc ld.lld qemu-riscv32 python3; do
    command -v "$tool" >/dev/null || {
        echo "Required tool not found: $tool" >&2
        exit 1
    }
done

export LLVM_PROFILE_FILE="$SCRIPT_DIR/far-jumps-no-scratch-%p.profraw"

# the compiler writes an image into the working directory
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

python3 - <<'EOF'
depth = 14
source = ["dat a = []{1, 2}", "func main() {", "    var x = 100"]
for level in range(depth):
    source.append("    " * (level + 1) + "foo a {")
source.append("        x = x + i")
source.append("        if x == 0 {")
source.append("            x = x + 1\n" * 90000 + "        }")
for level in reversed(range(depth)):
    source.append("    " * (level + 1) + "}")
source.append("}")
open("far.baz", "w").write("\n".join(source) + "\n")
EOF

echo -n "far jumps without scratch register rv32i: "
set +e
"$BIN" far.baz --target=rv32i --bin=far.bin >far.s 2>err
status=$?
set -e
[[ $status -ne 0 ]]
grep -q "exceeds 1 MiB and no scratch register is free" err
echo ok

# 'cnt' is a run-time count, so the compare of 'x > 2' holds the last free
# registers and only the jump of 'break' after it has a scratch register
python3 - <<'EOF'
depth = 13
source = [
    "dat a = []{1}",
    "func main() {",
    "    var x = 0",
    "    var y = 0",
    "    var cnt = 1",
]
for level in range(depth):
    head = "foo a, cnt {" if level == 0 else "foo a {"
    source.append("    " * (level + 1) + head)
source.append("        loop {")
source.append("            x = x + 1")
source.append("            if x > 2 break")
source.append("            y = y + 1\n" * 90000 + "        }")
for level in reversed(range(depth)):
    source.append("    " * (level + 1) + "}")
source.append("    exit(x * 10 + y)")
source.append("}")
open("far.baz", "w").write("\n".join(source) + "\n")
EOF

for optimize in "" "--nopt"; do
    echo -n "far jumps inverted branch rv32i $optimize: "
    "$BIN" far.baz --target=rv32i --bin=far.bin $optimize >far.s
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj far.s -o far.o
    ld.lld -m elf32lriscv -e _start -o far far.o
    set +e
    timeout -k 1s 60s qemu-riscv32 ./far
    status=$?
    set -e
    # note: 62 is the low byte of 'x * 10 + y', 3 * 10 + 180000
    [[ $status -eq 62 ]]
    echo ok
done
