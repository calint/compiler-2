#!/bin/bash
# runs the cases of 'cases.sh' that run the program, except the '--nopt'
# diffs, with the first and then the last of two versions of the same code
# kept ('--versions'), so the code of a version that the default choice drops
# is run and checked too
#
# usage: test-versions.sh
set -e

cd "$(dirname "$0")"

for versions in first last; do
    for target in x86_64 rv32i-fpga; do
        VERSIONS="$versions" ./test-coverage.sh --target="$target" run
    done
done
