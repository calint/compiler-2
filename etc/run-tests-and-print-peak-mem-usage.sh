#!/bin/sh
#set -e
cd $(dirname "$0")

cd ..

for file in qa/coverage/*.baz; do
  echo -n "$file: "
  valgrind --tool=massif --massif-out-file=massif.out ./baz "$file" >/dev/null 2>/dev/null
  grep mem_heap_B massif.out | sort -g | tail -n 1
  rm massif.out
done
