#!/bin/sh
set -e
cd $(dirname "$0")

cd ..

for file in qa/coverage/*.baz; do
  echo "File: $file"
  ./baz "$file" | tail -n 3
done
