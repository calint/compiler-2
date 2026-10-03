#!/bin/bash
# checks the diagnostic of an rv32i jump beyond 1 MiB when no scratch register
# is free for its far form: a loop nested 14 deep with a body of 1.08 MiB
#
# the body takes minutes to compile with the coverage build, so the check is a
# script of its own
#
# usage: test-far-jumps-no-scratch.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"

command -v python3 >/dev/null || {
    echo "Required tool not found: python3" >&2
    exit 1
}

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
