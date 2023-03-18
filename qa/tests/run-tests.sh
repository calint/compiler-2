#/bin/sh

RUN='echo -n "$SRC: " && $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen; e=$?;if test $e -eq $EXP; then echo ok; else echo FAILED. expected $EXP got $e; exit 1; fi'
DIFF='echo -n "$SRC: " && $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
BIN=../../baz

SRC=t1.baz && EXP=58 && eval $RUN
SRC=t2.baz && EXP=1 && eval $RUN
SRC=t3.baz && EXP=7 && eval $RUN
SRC=t4.baz && EXP=0 && eval $RUN
SRC=t5.baz && EXP=0 && eval $RUN
SRC=t6.baz && EXP=0 && eval $RUN
SRC=t7.baz && EXP=0 && eval $RUN
SRC=t8.baz && EXP=0 && eval $RUN
SRC=t9.baz && EXP=0 && eval $RUN
SRC=t10.baz && EXP=0 && eval $RUN
SRC=t11.baz && EXP=14 && eval $RUN
SRC=t12.baz && EXP=24 && eval $RUN
SRC=t13.baz && EXP=2 && eval $RUN
SRC=t14.baz && EXP=0 && eval $RUN
SRC=t15.baz && eval $DIFF
SRC=t16.baz && eval $DIFF
SRC=t17.baz && eval $DIFF
SRC=t18.baz && eval $DIFF
SRC=t19.baz && EXP=120 && eval $RUN
SRC=t20.baz && EXP=7 && eval $RUN
SRC=t21.baz && EXP=12 && eval $RUN
SRC=t22.baz && EXP=16 && eval $RUN
SRC=t23.baz && EXP=17 && eval $RUN
SRC=t24.baz && EXP=7 && eval $RUN
SRC=t25.baz && EXP=0 && eval $RUN
SRC=t26.baz && EXP=0 && eval $RUN
SRC=t27.baz && EXP=0 && eval $RUN

rm gen gen.o gen.s diff.baz out