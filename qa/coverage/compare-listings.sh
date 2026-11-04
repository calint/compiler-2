#!/bin/bash
# compares two builds of the compiler over every case of 'cases.sh': each case
# is compiled with its options and with '--checks=noub' for x86_64 and rv32i,
# and the listing, the error output, the exit code and the rv32i image of the
# two builds must be equal; the refactor check of AGENTS.md, e.g. a build of
# HEAD against the build of the working tree
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

cd "$(dirname "$0")"

TESTS=$(realpath tests)
DEFAULT_OPTIONS="--vars=0x40000 --checks=upper,lower,line"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# each case and its options, once per distinct pair
grep -oE '^ *SRC=[0-9]+ .*' cases.sh | sed -E 's/^ *//' | while read -r line; do
    src=$(sed -E 's/^SRC=([0-9]+).*/\1/' <<<"$line")
    options=$DEFAULT_OPTIONS
    if [[ "$line" =~ OPTS=\"([^\"]*)\" ]]; then
        options=${BASH_REMATCH[1]}
    fi
    printf '%s\t%s\n' "$src" "$options"
done | sort -u >"$WORK/jobs"

# compiles one case with both builds for both targets and both option sets
compare_case() {
    local src=$1
    local options=$2
    for target in x86_64 rv32i; do
        for opts in "$options" "--checks=noub"; do
            local dir
            dir=$(mktemp -d "$WORK/case.XXXXXX")
            (
                cd "$dir"
                # shellcheck disable=SC2086 # the options are words
                timeout 60 "$BASE" "$TESTS/$src.baz" --target=$target \
                    --bin=base.bin $opts >base.s 2>base.err && echo 0 >base.rc ||
                    echo $? >base.rc
                # shellcheck disable=SC2086
                timeout 60 "$NEW" "$TESTS/$src.baz" --target=$target \
                    --bin=new.bin $opts >new.s 2>new.err && echo 0 >new.rc ||
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
