#!/bin/sh
# runs every fuzzer of this directory, see 'README.md'
# usage: [FUZZ_TARGETS=all] fuzz-all.sh [scale] [seed]
# 'scale' multiplies the number of programs of each fuzzer (default 1, about
# 10 minutes), 'seed' makes a run repeatable, the run prints it first; the
# programs run on x86_64, rv32i-fpga and one of rv32i and rv32i-qemu, with
# 'FUZZ_TARGETS=all' on all four, see 'fuzzlib.with_rotated'; 'fuzz-ub.sh
# --mutants' on the first two
set -eu
cd "$(dirname "$0")"

scale=${1:-1}
seed=${2:-$(date +%s)}
echo "=== fuzz-all.sh $scale $seed"

for tool in nasm ld llvm-mc ld.lld llvm-objcopy qemu-riscv32 \
    qemu-system-riscv32; do
    command -v "$tool" >/dev/null || {
        echo "fuzz-all.sh: '$tool' is missing" >&2
        exit 1
    }
done
[ -x ../../baz ] || ../../make.sh build
# rebuilt when missing or older than a source
if [ ! -x ../../fpga-emulator/osqa ] || [ -n "$(find ../../fpga-emulator/src -newer ../../fpga-emulator/osqa -print -quit)" ]; then
    ../../fpga-emulator/make.sh
fi
./build-asan.sh

# the work directories of the fuzzers, also those of a run that is stopped;
# the findings stay in 'findings/'
TMPDIR=$(mktemp -d -t fuzz-all.XXXXXX)
export TMPDIR
trap 'rm -rf -- "$TMPDIR"' EXIT
trap 'exit 130' INT TERM HUP

status=0
run() {
    echo "=== $*"
    "$@" || status=1
}

# the coverage of the generators is checked with its own fixed seed, so a
# rare construct does not fail the run by chance
run ./fuzz-coverage.py 100
run ./fuzz-constructs.py "$scale" "$seed"
run ./fuzz-ub.py $((200 * scale)) "$seed"
run ./fuzz-exprs.py $((3000 * scale)) "$seed"
run ./fuzz-data.py $((300 * scale)) "$seed"
run ./fuzz-flow.py $((300 * scale)) "$seed"
run ./fuzz-bools.py $((300 * scale)) "$seed"
run ./fuzz-calls.py $((300 * scale)) "$seed"
run ./fuzz-effects.py $((300 * scale)) "$seed"
run ./fuzz-methods.py $((300 * scale)) "$seed"
run ./fuzz-generics.py $((300 * scale)) "$seed"
run ./fuzz-reject.py $((600 * scale)) "$seed"
run ./fuzz-io.py $((300 * scale)) "$seed"
run ./fuzz-programs.py $((500 * scale)) "$seed"
run ./fuzz-limits.py $((250 * scale)) "$seed"
run ./fuzz-whole.py $((200 * scale)) "$seed"
run ./fuzz-versions.py $((150 * scale)) "$seed"
run ./fuzz-frames.py $((1500 * scale)) "$seed"
run ./fuzz-combos.py $((200 * scale)) "$seed"
run ./fuzz-compiler.py $((3000 * scale)) "$seed"
run ./fuzz-mutate.py $((15000 * scale)) "$seed"
run ./fuzz-options.py $((3000 * scale)) "$seed"
run ./fuzz-roome.py $((3000 * scale)) "$seed"
run ./fuzz-alias.sh $((2000000 * scale)) $((5000 * scale)) "$seed"
run ./fuzz-ub.sh --mutants

echo "=== findings are in findings/"
exit $status
