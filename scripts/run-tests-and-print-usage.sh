#!/bin/sh
set -e
cd $(dirname "$0")

cd ..

for file in qa/coverage/tests/*.baz; do
    echo "File: $file"
    ./baz "$file" 2>/dev/null | tail -n 6
done
