#!/bin/sh
# runs every fuzzer of this directory, see 'README.md'
# usage: fuzz-all.sh [scale] [seed]
# 'scale' multiplies the number of programs of each fuzzer (default 1, about
# 10 minutes), 'seed' makes a run repeatable
set -eu
cd "$(dirname "$0")"

scale=${1:-1}
seed=${2:-$(date +%s)}

[ -x ../../baz ] || ../../make.sh build
[ -x ../../fpga-emulator/osqa ] || ../../fpga-emulator/make.sh
./build-asan.sh

status=0
run() {
    echo "=== $*"
    "$@" || status=1
}

run ./fuzz-ub.py $((200 * scale)) "$seed"
run ./fuzz-exprs.py $((3000 * scale)) "$seed"
run ./fuzz-programs.py $((500 * scale)) "$seed"
run ./fuzz-compiler.py $((3000 * scale)) "$seed"
run ./fuzz-options.py $((3000 * scale)) "$seed"
run ./fuzz-roome.py $((3000 * scale)) "$seed"
run ./fuzz-alias.sh

echo "=== findings are in findings/"
exit $status
