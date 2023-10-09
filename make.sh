#!/bin/sh
# tools:
#   clang++: 15.0.7
#       g++: 12.3.0
#      nasm: 2.16.01
#        ld: 2.40

set -e

# change to directory of the script
cd $(dirname "$0")

# set environment
CC="clang++ -std=c++20"
CF="-Werror -Wfatal-errors"
CW="-Weverything -Wno-c++98-compat -Wno-weak-vtables  -Wno-padded \
    -Wno-unqualified-std-cast-call"
MSAN=
PROF=
DBG=
OPT=-O3
if [ "$1" = "msan" ]; then
#    MSAN="-fsanitize=memory,undefined -fsanitize-memory-track-origins=2 -fsanitize-ignorelist=msan_suppressions.txt"
    MSAN="-fsanitize=address,undefined -fno-omit-frame-pointer"
    DBG=-g
elif [ "$1" = "prof" ]; then
    # profiling, sanitize
    MSAN="-fsanitize=address,undefined -fno-omit-frame-pointer"
    PROF="-fprofile-instr-generate -fcoverage-mapping"
    DBG=-g
fi

SEP="--------------------------------------------------------------------------------"

CMD="$CC src/main.cpp -o baz $OPT $DBG $CF $CW $MSAN $PROF"
echo $SEP
echo $CMD
$CMD
echo $SEP


if [ "$1" = "prof" ]; then
    exit 0
fi

./baz prog.baz > gen.s
grep -v -e'^\s*;.*$' -e'^\s*$' gen.s > gen-m.s
nasm -f elf64 gen.s
ld -s -o gen gen.o

ls --color -la baz gen.s gen-m.s gen

echo $SEP
echo    '              lines   words   chars'
echo -n '    source: '; cat src/* | wc
echo -n '   gzipped: '; cat src/* | gzip | wc 
echo -n 'baz source: '; cat prog.baz | grep -v -e'^\s*$' | wc
echo -n '   gzipped: '; cat prog.baz | grep -v -e'^\s*$' | gzip | wc
echo -n 'asm source: '; cat gen-m.s | wc
echo -n '   gzipped: '; cat gen-m.s | gzip | wc
echo $SEP
./gen
echo $SEP
