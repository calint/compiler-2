#!/bin/sh
set -e
cd $(dirname "$0")

cd ..

for file in qa/coverage/*.baz; do
  echo "File: $file"
  ./baz "$file" 2>/dev/null | tail -n 5
done
