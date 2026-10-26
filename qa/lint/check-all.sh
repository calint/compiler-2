#!/bin/sh
# runs every 'check-*.py' of this directory: the design rules that are not
# compiler warnings, e.g. the include layers, the quoting of messages, the
# names of the README and a test for each compile error
# usage: qa/lint/check-all.sh
# exits 1 after the first check that fails
set -eu
cd "$(dirname "$0")"

for check in check-*.py; do
    echo "$check"
    "./$check"
done
