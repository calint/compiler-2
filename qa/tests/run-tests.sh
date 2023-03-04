#/bin/sh

BIN=../../compi

SRC=t1.baz
echo -n "$SRC: " && $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary; e=$?
if test $e -eq 58; then echo ok; else echo nok. expected 58 got $e; exit 1; fi

SRC=t2.baz
echo -n "$SRC: " && $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary; e=$?
if test $e -eq 1; then echo ok; else echo nok. expected 1 got $e; exit 1; fi

SRC=t3.baz
echo -n "$SRC: " && $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o binary gen.o && ./binary; e=$?
if test $e -eq 7; then echo ok; else echo nok. expected 7 got $e; exit 1; fi

rm binary gen.o gen.s diff.baz