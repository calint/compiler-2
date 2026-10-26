#!/bin/sh
# differential fuzzer of the undefined behavior checks, see 'fuzz-ub.py'
# usage: fuzz-ub.sh [count] [seed] [scenario]
#        fuzz-ub.sh --mutants
# '--mutants' turns off one check at a time and requires the fuzzer to find the
# program that the missing check lets through
set -eu
cd "$(dirname "$0")"

# the sanitizer build that 'fuzzlib' runs, 'fuzzlib.assert_fresh' stops a
# stale one
[ -x build/baz-asan ] || ./build-asan.sh
[ -x ../../fpga-emulator/osqa ] || ../../fpga-emulator/make.sh

if [ "${1:-}" != "--mutants" ]; then
    ./fuzz-ub.py "$@"
    exit
fi

missed=0
# the mismatches of a mutant are expected, they are not findings
FUZZ_FINDINGS=$(mktemp -d)
export FUZZ_FINDINGS
# check turned off, scenario that must catch it, number of programs
while read -r checks scenario count; do
    # caught only by a mismatch, a fuzzer that fails otherwise caught nothing
    output=$(FUZZ_CHECKS="noub,$checks" ./fuzz-ub.py "$count" 1 "$scenario" \
        2>&1) || true
    if printf '%s\n' "$output" | grep -q '^MISMATCH '; then
        echo "caught: $checks ($scenario)"
    else
        echo "not caught: $checks ($scenario)"
        printf '%s\n' "$output" | tail -n 3
        missed=1
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
rm -rf -- "$FUZZ_FINDINGS"
exit $missed
