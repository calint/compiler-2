#/bin/sh
RUN='echo -n "$SRC: " && $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen; e=$?;if test $e -eq $EXP; then echo ok; else echo nok. expected $EXP got $e; exit 1; fi'
BIN=../../baz

SRC=t1.baz
EXP=58
eval $RUN

SRC=t2.baz
EXP=1
eval $RUN

SRC=t3.baz
EXP=7
eval $RUN

SRC=t4.baz
EXP=0
eval $RUN

SRC=t5.baz
EXP=0
eval $RUN

SRC=t6.baz
EXP=0
eval $RUN

rm gen gen.o gen.s diff.baz