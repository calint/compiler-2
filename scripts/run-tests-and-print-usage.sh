#!/bin/sh
# prints the last lines of the footer of the listing of each test source: the
# sizes of data and variables and the instruction count
# usage: scripts/run-tests-and-print-usage.sh
set -eu
cd "$(dirname "$0")"

cd ..

for file in qa/coverage/tests/*.baz; do
    echo "File: $file"
    ./baz "$file" 2>/dev/null | tail -n 6
done
