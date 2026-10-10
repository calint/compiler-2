#!/bin/sh
# differential fuzzer of the undefined behavior checks, see 'fuzz-ub.py'
# usage: fuzz-ub.sh [count] [seed] [scenario]
#        fuzz-ub.sh --mutants
# '--mutants' turns off one check at a time and requires the fuzzer to find the
# program that the missing check lets through
set -eu
cd "$(dirname "$0")"

[ -x ../../baz ] || ../../make.sh build
[ -x ../../fpga-emulator/osqa ] || ../../fpga-emulator/make.sh

if [ "${1:-}" != "--mutants" ]; then
    ./fuzz-ub.py "$@"
    exit
fi

missed=0
# check turned off, scenario that must catch it, number of programs
while read -r checks scenario count; do
    if FUZZ_CHECKS="noub,$checks" ./fuzz-ub.py "$count" 1 "$scenario" \
        >/dev/null; then
        echo "not caught: $checks ($scenario)"
        missed=1
    else
        echo "caught: $checks ($scenario)"
    fi
done <<EOF
-overflow arith 40
-division arith 60
-shift arith 60
-shift shifts 100
-overflow contexts 200
-division contexts 300
-shift contexts 300
-upper contexts 300
-lower contexts 300
-upper arrays 60
-lower arrays 60
-overlap arrays 100
-upper indexes 100
-lower indexes 100
-overlap indexes 300
-frame,-stack deep 10
EOF
exit $missed
