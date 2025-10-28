#!/bin/bash
set -e
cd $(dirname "$0")

SEP="--------------------------------------------------------------------------------"
echo $SEP
CMD="./baz $@"
echo $CMD
$CMD >gen.s
echo $SEP
grep -v -e'^\s*;.*$' -e'^\s*$' gen.s >gen-without-comments.s
grep -v -e'^\s*;.*$' -e'^\s*$' -e'^.*:\s*$' gen.s | wc | awk '{print "instructions: " $1}'
grep -E '^\s*jmp\s.*.*$' gen.s | wc | awk '{print "jmp: " $1}'
grep -E '^\s*j[a-z]{1,2}\s' gen.s | grep -v '^\s*jmp\s' | wc | awk '{print "jcc: " $1}'
echo $SEP
nasm -f elf64 gen.s
ld -s -o gen gen.o
ls --color -la gen.s gen-without-comments.s gen
echo $SEP
set +e # don't stop att errors
./gen $PRG $STK $CHK
RET=$?
echo $SEP
echo returned: $RET
