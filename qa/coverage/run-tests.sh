#/bin/sh

# change to directory of the script
cd $(dirname "$0")

../../make.sh prof 

RUN='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen; e=$?;if test $e -eq $EXP; then echo ok; else echo FAILED. expected $EXP got $e; exit 1; fi'
DIFF='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
DIFFINP='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen < ${SRC%.*}.in > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
DIFFPY='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./${SRC%.*}.py > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
#BIN='valgrind --quiet --leak-check=full --show-leak-kinds=all -s ../../baz'
BIN='../../baz'

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
SRC=t28.baz && EXP=0 && eval $RUN
SRC=t29.baz && EXP=0 && eval $RUN
SRC=t30.baz && EXP=0 && eval $RUN
SRC=t31.baz && EXP=0 && eval $RUN
SRC=t32.baz && EXP=0 && eval $RUN
SRC=t33.baz && EXP=0 && eval $RUN
SRC=t34.baz && EXP=0 && eval $RUN
SRC=t35.baz && EXP=0 && eval $RUN
SRC=t36.baz && eval $DIFFINP
SRC=t37.baz && EXP=0 && eval $RUN
SRC=t38.baz && EXP=0 && eval $RUN
SRC=t39.baz && EXP=0 && eval $RUN
SRC=t40.baz && EXP=0 && eval $RUN
SRC=t41.baz && EXP=0 && eval $RUN
SRC=t42.baz && EXP=0 && eval $RUN
SRC=t43.baz && EXP=0 && eval $RUN
SRC=t44.baz && EXP=0 && eval $RUN
SRC=t45.baz && EXP=0 && eval $RUN
SRC=t46.baz && EXP=0 && eval $RUN
SRC=t47.baz && EXP=0 && eval $RUN
SRC=t48.baz && EXP=0 && eval $RUN
SRC=t49.baz && EXP=0 && eval $RUN
SRC=t50.baz && EXP=0 && eval $RUN
SRC=t51.baz && EXP=0 && eval $RUN
SRC=t52.baz && EXP=0 && eval $RUN
SRC=t53.baz && EXP=0 && eval $RUN
SRC=t54.baz && EXP=0 && eval $RUN
SRC=t55.baz && EXP=0 && eval $RUN
SRC=t56.baz && EXP=0 && eval $RUN
SRC=t57.baz && EXP=0 && eval $RUN
SRC=t58.baz && EXP=0 && eval $RUN
SRC=t59.baz && EXP=0 && eval $RUN
SRC=t60.baz && EXP=0 && eval $RUN
SRC=t61.baz && EXP=0 && eval $RUN
SRC=t62.baz && EXP=0 && eval $RUN
SRC=t63.baz && eval $DIFFPY
SRC=t64.baz && EXP=0 && eval $RUN

rm gen gen.o gen.s diff.baz out

# process coverage data
llvm-profdata merge -o baz.profdata -sparse $(ls *.profraw) &&
llvm-cov export --format=lcov --instr-profile baz.profdata --object $BIN > lcov.info &&
# generate report
genhtml --quiet lcov.info --output-directory report/ &&
echo &&
echo coverage report generated in "$(realpath "report/")" &&
echo &&

rm *.profraw baz.profdata lcov.info
