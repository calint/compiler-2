#!/bin/bash

set -e

PRG=${1:-prog.baz}

# change to directory of the script
cd $(dirname "$0")

SEP="--------------------------------------------------------------------------------"
echo $SEP
./baz $PRG >gen.s &&
  nasm -f elf64 gen.s &&
  ld -s -o gen gen.o &&
  cat gen.s | grep -v -e'^\s*;.*$' -e'^\s*$' >gen-without-comments.s &&
  ls --color -la gen.s gen-without-comments.s gen &&
  echo $SEP &&
  ./gen &&
  echo $SEP
echo returned: $?
