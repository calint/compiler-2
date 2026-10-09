#!/bin/sh
# long run of the aliasing fuzz: one job per core, each with its own seed, large
# counts, to run before a change to the aliasing check is accepted, takes a few
# minutes (needs a built '../../baz', see 'make.sh build')
# usage: qa/fuzz/fuzz-alias-long.sh [jobs] [first-seed] [paths] [programs]
#   jobs        parallel jobs, one seed each (default: the number of cores)
#   first-seed  seed of the first job, the others follow (default 1000)
#   paths       pairs of accesses per job for the C++ fuzz (default 30000000)
#   programs    programs per job for the compiler fuzz (default 150000)
# the output of a job is kept in a file and shown only when the job fails
set -eu
cd "$(dirname "$0")"

JOBS="${1:-$(nproc)}"
FIRST_SEED="${2:-1000}"
PATHS="${3:-30000000}"
PROGRAMS="${4:-150000}"

TMP="$(mktemp -d /tmp/fuzz-alias-long.XXXXXX)"
trap 'rm -rf "$TMP"' EXIT

echo "alias fuzz long: $JOBS jobs from seed $FIRST_SEED, $PATHS paths and $PROGRAMS programs each"

clang++ -std=c++26 -O2 -Wno-braced-scalar-init alias-path-fuzz.cpp \
    -o "$TMP/alias-path-fuzz"

job=0
while [ "$job" -lt "$JOBS" ]; do
    seed=$((FIRST_SEED + job))
    (
        status=0
        "$TMP/alias-path-fuzz" "$PATHS" "$seed" >"$TMP/$job.out" 2>&1 || status=1
        ./alias-foo-fuzz.py "$PROGRAMS" "$seed" ../../baz >>"$TMP/$job.out" 2>&1 ||
            status=1
        echo "$status" >"$TMP/$job.status"
    ) &
    job=$((job + 1))
done
wait

failed=0
job=0
while [ "$job" -lt "$JOBS" ]; do
    seed=$((FIRST_SEED + job))
    if [ "$(cat "$TMP/$job.status")" = 0 ]; then
        echo "seed $seed: ok"
    else
        echo "seed $seed: FAILED"
        cat "$TMP/$job.out"
        failed=1
    fi
    job=$((job + 1))
done

if [ "$failed" = 0 ]; then
    echo "alias fuzz long: done"
fi
exit "$failed"
