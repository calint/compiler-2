#!/bin/bash
# compares two builds of the compiler over every case of 'cases.sh': each case
# is compiled with its options and with '--checks=noub,line
# --report=registers' (a refactor keeps the line handlers and the report too)
# for every target, the uart targets for their input and output routines, and
# the listing, the error output, the exit code and the rv32i image of the two
# builds must be equal; the refactor check of AGENTS.md, e.g. a build of HEAD
# against the build of the working tree
#
# usage: compare-listings.sh BASE_BAZ NEW_BAZ
#   prints the cases that differ and their count, exits 1 when any differs
set -eu
set -o pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 BASE_BAZ NEW_BAZ" >&2
    exit 2
fi

BASE=$(realpath "$1")
NEW=$(realpath "$2")
# a build that cannot run fails each compile alike, so nothing would differ
for build in "$BASE" "$NEW"; do
    if [[ ! -f $build || ! -x $build ]]; then
        echo "$0: '$build' is not an executable" >&2
        exit 2
    fi
done

cd "$(dirname "$0")"

TESTS=$(realpath tests)
# the options of a case without its own, as in 'test-coverage.sh'
DEFAULT_OPTIONS="--vars=0x40000 --checks=upper,lower,line"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# each case and its options, once per distinct pair; a case may follow an
# 'if' on the target, a commented-out one is not compiled
grep -vE '^ *#' cases.sh | grep -oE 'SRC=[0-9]+[^;]*' | while read -r line; do
    src=$(sed -E 's/^SRC=([0-9]+).*/\1/' <<<"$line")
    options=$DEFAULT_OPTIONS
    if [[ "$line" =~ OPTS=\"([^\"]*)\" ]]; then
        options=${BASH_REMATCH[1]}
    fi
    printf '%s\t%s\n' "$src" "$options"
done | sort -u >"$WORK/jobs"

# compiles one case with both builds for every target and both option sets
compare_case() {
    local src=$1
    local options=$2
    for target in x86_64 rv32i rv32i-qemu rv32i-fpga; do
        for opts in "$options" "--checks=noub,line --report=registers"; do
            local dir
            dir=$(mktemp -d "$WORK/case.XXXXXX")
            # the rv32i targets write an image, x86_64 takes no '--bin'
            local base_image=() new_image=()
            if [[ $target != x86_64 ]]; then
                base_image=(--bin=base.bin)
                new_image=(--bin=new.bin)
            fi
            (
                cd "$dir"
                # shellcheck disable=SC2086 # the options are words
                timeout 60 "$BASE" "$TESTS/$src.baz" --target=$target \
                    "${base_image[@]}" $opts >base.s 2>base.err && echo 0 >base.rc ||
                    echo $? >base.rc
                # shellcheck disable=SC2086
                timeout 60 "$NEW" "$TESTS/$src.baz" --target=$target \
                    "${new_image[@]}" $opts >new.s 2>new.err && echo 0 >new.rc ||
                    echo $? >new.rc
                sed -i "s|$BASE|BAZ|g;s|$NEW|BAZ|g" base.err new.err
                if ! cmp -s base.s new.s || ! cmp -s base.err new.err ||
                    ! cmp -s base.rc new.rc ||
                    { [[ -f base.bin || -f new.bin ]] &&
                        ! cmp -s base.bin new.bin; }; then
                    echo "differs: $src $target [$opts]"
                fi
            )
            rm -rf "$dir"
        done
    done
}
export -f compare_case
export BASE NEW TESTS WORK

tr '\t' '\n' <"$WORK/jobs" |
    xargs -d '\n' -n2 -P"$(nproc)" bash -c 'compare_case "$0" "$1"' |
    sort >"$WORK/differs"

cat "$WORK/differs"
echo "cases: $(wc -l <"$WORK/jobs"), differing compiles: $(wc -l <"$WORK/differs")"

[[ ! -s "$WORK/differs" ]]
