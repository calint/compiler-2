#/bin/sh

BIN=../../compi

echo -n "t1.baz: "
$BIN t1.baz > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary; e=$?
if test $e -eq 58; then echo ok; else echo nok. expected 58 got $e; exit 1; fi

echo -n "t2.baz: "
$BIN t2.baz > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary; e=$?
if test $e -eq 1; then echo ok; else echo nok. expected 1 got $e; exit 1; fi

rm binary gen.o gen.s diff.baz