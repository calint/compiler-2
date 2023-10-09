#!/bin/sh
echo "---------------------------------------------------------" &&
./baz prog.baz > gen.s &&
nasm -f elf64 gen.s &&
ld -s -o gen gen.o &&
cat gen.s|grep -v -e'^\s*;.*$' -e'^\s*$' > gen-m.s &&
ls --color -la baz gen.s gen-m.s gen &&
echo "---------------------------------------------------------" &&
./gen
echo "---------------------------------------------------------" &&
echo returned: $?
