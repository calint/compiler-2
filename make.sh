#!/bin/sh
# tools:
#   clang++: Ubuntu clang version 15.0.6 x86_64-pc-linux-gnu posix
#      nasm: NASM version 2.15.05
#        ld: GNU ld (GNU Binutils for Ubuntu) 2.39
#       g++: g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0
set -e

CC="clang++ -std=c++2a"
CF="-Os -fno-inline -Werror -Wfatal-errors"
CW="-Weverything -Wno-c++98-compat -Wno-weak-vtables -Wno-unqualified-std-cast-call -Wno-padded -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-private-field"

$CC $CF $CW -o baz src/main.cpp

./baz prog.baz > gen.s
nasm -f elf64 gen.s
ld -s -o gen gen.o

ls --color -la baz gen.s gen-m.s gen

cat gen.s|grep -v -e'^\s*;.*$' -e'^\s*$' > gen-m.s

echo
echo -n '    source: ' && cat src/*|wc
echo -n '   gzipped: ' && cat src/*|gzip|wc 
echo -n 'baz source: ' && cat prog.baz|grep -v -e'^\s*$'|wc
echo -n '   gzipped: ' && cat prog.baz|grep -v -e'^\s*$'|gzip|wc
echo -n 'asm source: ' && cat gen-m.s|wc
echo -n '   gzipped: ' && cat gen-m.s|gzip|wc
echo
./gen
