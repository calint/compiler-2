#!/bin/sh
#set -e
cd $(dirname "$0")

cd ../qa/coverage/

for file in *.baz; do
  echo -n "$file: "
  valgrind --tool=massif --massif-out-file=massif.out ../../baz "$file" >/dev/null 2>/dev/null
  grep mem_heap_B massif.out | sort -r | head -n 1 | awk '{printf "%s ", $2; print $1}'
  rm massif.out
done
