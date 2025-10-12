#!/bin/bash
set -e
cd $(dirname "$0")

PRG=${1:-prog.baz}
STK=${2:-131072}
CHK=${3:-checked}
CHKLINE=${4:-line}
SEP="..  . .. . . . . .  ......... . ..  . .  . . ....  . .. ....... . . "
echo $SEP
CMD="./baz $PRG $STK $CHK $CHKLINE"
echo $CMD
$CMD >gen.s
echo $SEP
grep -v -e'^\s*;.*$' -e'^\s*$' gen.s >gen-without-comments.s
nasm -f elf64 gen.s
ld -s -o gen gen.o
ls --color -la gen.s gen-without-comments.s gen
echo $SEP
set +e # don't stop att errors
./gen $PRG $STK $CHK
RET=$?
echo $SEP
echo returned: $RET
