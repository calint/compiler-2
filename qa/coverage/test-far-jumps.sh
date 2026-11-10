#!/bin/bash
# checks the rv32i jumps that grow with the code between them: programs whose
# 'foo', 'if', 'break' and 'continue' jump over bodies of 8.4 KiB (a branch
# becomes 'j') and 1.08 MiB (a jump becomes 'jump'), compiled with and without
# the jump optimizer, assembled, linked and executed under qemu; and that a
# grown conditional jump, whose 'j' follows the inverted branch, keeps 'j' just
# inside the reach of 'jal' and becomes 'jump' just outside it, forward and
# backward
#
# usage: test-far-jumps.sh [short]
#   short   only the 8.4 KiB bodies, which compile in seconds
set -eE
# names the failed command; those under 'set +e' fail on purpose
trap '[[ $- != *e* ]] || echo "FAILED. line $LINENO: $BASH_COMMAND" >&2' ERR

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"

for tool in llvm-mc llvm-objdump ld.lld qemu-riscv32 python3; do
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

# writes 'far.baz': each 'pad = pad + 1' is 8 bytes (its load is a removed
# reload of the store before it), so 1050 needs 'j' and 135000 needs 'jump'
# for the jumps of 'foo', 'if', 'break' and 'continue'
generate_foo() {
    python3 - "$@" <<'EOF'
import sys
counts = [int(count) for count in sys.argv[1:]]
source = [
    "dat values = []{1, 2, 3, 4, 5}",
    "func main() {",
    "    var pad = 0",
    "    var visits = 0",
    "    var sum = 0",
]
for count in counts:
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

if [[ ${1:-} == short ]]; then
    generate_foo 1050
else
    generate_foo 1050 135000
fi
for optimize in "" "--nopt"; do
    echo -n "far jumps foo rv32i $optimize: "
    "$BIN" far.baz --target=rv32i --bin=far.bin $optimize >far.s
    grep -qE '^ +j foo\.[0-9]+\.[0-9]+$' far.s
    grep -qE '^ +j if\.[0-9]+\.[0-9]+\.end$' far.s
    if [[ ${1:-} != short ]]; then
        grep -qE '^ +jump foo\.[0-9]+\.[0-9]+, ' far.s
        grep -qE '^ +jump foo\.[0-9]+\.[0-9]+\.continue, ' far.s
        grep -qE '^ +jump foo\.[0-9]+\.[0-9]+\.end, ' far.s
        grep -qE '^ +jump if\.[0-9]+\.[0-9]+\.end, ' far.s
    fi
    # a far conditional branch jumps over the far jump with the inverse branch
    grep -qE '^ +b[a-z]+ .*, \.Lbaz_jump\.[0-9]+$' far.s
    assemble_and_run
    echo ok
done

if [[ ${1:-} == short ]]; then
    exit 0
fi

# writes 'far.baz' and 'far.labels': two forward conditional jumps ('if' to
# its end) and two backward ('continue' to the start of the loop) over bodies
# of the given counts of 'pad = 0'; each line of 'far.labels' names the
# direction of a jump, the label after its line and its target
generate_boundary() {
    python3 - "$@" <<'EOF'
import sys
counts = [int(count) for count in sys.argv[1:]]
source = [
    "func main() {",
    "    var pad = 0",
    "    var k = 0",
    "    var visits = 0",
]
labels = []
for count in counts[:2]:
    line = len(source) + 1
    labels.append(f"forward if.{line}.5.code if.{line}.5.end")
    # 'k' is 0, so the jump over the body is taken
    source.append("    if k == 7 {")
    source.extend(["        pad = 0"] * count)
    source.append("        visits = visits + 10")
    source.append("    }")
for count in counts[2:]:
    source.append("    k = 0")
    loop_line = len(source) + 1
    source.append("    loop {")
    source.extend(["        pad = 0"] * count)
    source.append("        k = k + 1")
    source.append("        if k == 3 break")
    line = len(source) + 1
    labels.append(f"backward if.{line}.9.code loop.{loop_line}.5")
    # 'continue' at 1 and falls through at 2
    source.append("        if k != 2 continue")
    source.append("        visits = visits + 1")
    source.append("    }")
source.append("    if visits != 2 exit(1)")
source.append("}")
open("far.baz", "w").write("\n".join(source) + "\n")
open("far.labels", "w").write("\n".join(labels) + "\n")
EOF
}

# prints the span of each jump of 'far.labels' in 'far.o', which does not
# depend on the size of the jump: forward the bytes from the end of its line to
# the target, backward the bytes from the target to its branch, which starts
# its line
measure_boundary() {
    python3 - <<'EOF'
import re
import subprocess
def objdump(*args):
    return subprocess.run(["llvm-objdump", *args, "far.o"], check=True,
                          capture_output=True, text=True).stdout
symbols = {}
for line in objdump("-t").splitlines():
    fields = line.split()
    if len(fields) == 5 and fields[2] == ".text":
        symbols[fields[4]] = int(fields[0], 16)
spans = []
for line in open("far.labels"):
    direction, after, target = line.split()
    if direction == "forward":
        spans.append(symbols[target] - symbols[after])
        continue
    # note: 12 covers the longest line, the branch and 'jump'
    listing = objdump("-d", "--no-show-raw-insn",
                      f"--start-address={symbols[after] - 12}",
                      f"--stop-address={symbols[after]}")
    branches = [
        int(match[1], 16)
        for match in re.finditer(r"^ *([0-9a-f]+):\s+(b[a-z]+)\s", listing,
                                 re.MULTILINE)
    ]
    spans.append(branches[-1] - symbols[target])
print(*spans)
EOF
}

# the 'j' of a grown conditional jump starts 4 bytes into its 8-byte line and
# 'jal' reaches 1 MiB - 2 forward and 1 MiB backward, so these spans put the
# 'j' just inside and just outside its reach: forward 1 MiB - 8 and 1 MiB - 4,
# backward 1 MiB - 4 and 1 MiB
reach=$((1 << 20))
wanted=($((reach - 8)) $((reach - 4)) $((reach - 4)) $reach)

# a short build measures the code around the bodies, each 'pad = 0' is 4
# bytes
calibration=1000
generate_boundary $calibration $calibration $calibration $calibration
"$BIN" far.baz --target=rv32i --bin=far.bin >far.s
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj far.s -o far.o
read -ra spans < <(measure_boundary)
counts=()
for i in 0 1 2 3; do
    counts+=($((calibration + (wanted[i] - spans[i]) / 4)))
done

echo -n "far jumps conditional at the reach of jal rv32i: "
generate_boundary "${counts[@]}"
"$BIN" far.baz --target=rv32i --bin=far.bin >far.s
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj far.s -o far.o
read -ra spans < <(measure_boundary)
[[ ${spans[*]} == "${wanted[*]}" ]]
mapfile -t jumps < <(cut -d ' ' -f 3 far.labels)
grown="$(grep -A1 -E '^ +b[a-z]+ .*, \.Lbaz_jump\.[0-9]+$' far.s)"
grep -qE "^ +j ${jumps[0]}$" <<<"$grown"
grep -qE "^ +jump ${jumps[1]}, " <<<"$grown"
grep -qE "^ +j ${jumps[2]}$" <<<"$grown"
grep -qE "^ +jump ${jumps[3]}, " <<<"$grown"
assemble_and_run
echo ok
