#!/bin/sh
# fuzzes the aliasing check against a brute force of the bytes: the path
# comparison alone (C++), one seed per job, then random programs with the
# element 'e' of a 'foo' and with 'mut' parameters through the compiler (needs
# a built '../../baz', see 'make.sh build')
# usage: qa/fuzz/fuzz-alias.sh [paths] [programs] [seed] [jobs]
#   paths     pairs of accesses per job for the C++ fuzz (default 2000000)
#   programs  programs for the compiler fuzz (default 5000)
#   seed      random seed of the first job, the others follow (default 1)
#   jobs      parallel jobs (default: the number of cores)
# a long run, before a change to the aliasing check is accepted, takes a few
# minutes: fuzz-alias.sh 30000000 2000000 1000
set -eu
cd "$(dirname "$0")"

PATHS="${1:-2000000}"
PROGRAMS="${2:-5000}"
SEED="${3:-1}"
JOBS="${4:-$(nproc)}"

TMP="$(mktemp -d -t fuzz-alias.XXXXXX)"
# the jobs of a run that is stopped are stopped too
pids=""
trap 'kill $pids 2>/dev/null || true; rm -rf -- "$TMP"' EXIT
trap 'exit 130' INT TERM HUP

echo "alias fuzz: path comparison, $JOBS jobs from seed $SEED"
clang++ -std=c++26 -O2 -Wno-braced-scalar-init alias-path-fuzz.cpp \
    -o "$TMP/alias-path-fuzz"

job=0
while [ "$job" -lt "$JOBS" ]; do
    "$TMP/alias-path-fuzz" "$PATHS" $((SEED + job)) >"$TMP/$job.out" 2>&1 &
    pids="$pids $!"
    job=$((job + 1))
done

# the output of a job is shown only when it fails
status=0
job=0
for pid in $pids; do
    if ! wait "$pid"; then
        echo "seed $((SEED + job)): FAILED"
        cat "$TMP/$job.out"
        status=1
    fi
    job=$((job + 1))
done
[ "$status" -eq 0 ] || exit 1

echo "alias fuzz: programs"
./alias-foo-fuzz.py "$PROGRAMS" "$SEED" ../../baz "$JOBS"

echo "alias fuzz: done"
