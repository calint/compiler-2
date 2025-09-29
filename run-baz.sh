#!/bin/bash
set -e
cd $(dirname "$0")

PRG=${1:-prog.baz}
SEP="..  . .. . . . . .  ......... . ..  . .  . . ....  . .. ....... . . "
echo $SEP
./baz prog.baz >gen.s
ls --color -la gen.s gen-without-comments.s gen
grep -v -e'^\s*;.*$' -e'^\s*$' gen.s >gen-without-comments.s
nasm -f elf64 gen.s
ld -s -o gen gen.o
echo $SEP
set +e # don't stop att errors
./gen
RET=$?
echo $SEP
echo returned: $RET
