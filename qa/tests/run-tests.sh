#/bin/sh
set -s
BIN=../../compi

echo -n "t1.baz: "
$BIN t1.baz > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary
if test $? -eq 58; then echo ok; else echo nok. expected 58; exit 1; fi

echo -n "t2.baz: "
$BIN t2.baz > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary
if test $? -eq 1; then echo ok; else echo nok. expected 1; exit 1; fi

rm binary gen.o gen.s diff.baz