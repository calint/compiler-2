#!/bin/sh

CC="clang++ -std=c++2a"
CF="-Os -Wfatal-errors -fno-inline"
CW="-Weverything -pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wmissing-declarations -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo -Wswitch-default -Wundef -Wfloat-equal -Wsign-conversion -Wfloat-conversion -Wold-style-cast"
CW="$CW -Wno-c++98-compat -Wno-weak-vtables -Wno-unqualified-std-cast-call -Wno-padded -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-private-field"

$CC $CF $CW -Wfatal-errors -o baz src/main.cpp &&

./baz prog.baz > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o &&

ls --color -la baz gen.s gen &&

ASM_LINES="cat gen.s|grep -v -e'^;.*$' -e'^\s*$'"

echo &&
echo -n '    source: ' && cat src/*|wc &&
echo -n '   gzipped: ' && cat src/*|gzip|wc && 
echo -n 'baz source: ' && cat prog.baz|grep -v -e'^\s*$'|wc &&
echo -n '   gzipped: ' && cat prog.baz|grep -v -e'^\s*$'|gzip|wc &&
echo -n 'asm source: ' && cat src/*|eval $ASM_LINES|wc &&
echo -n '   gzipped: ' && cat src/*|eval $ASM_LINES|gzip|wc &&
echo &&

cat gen.s &&
echo &&
./gen
