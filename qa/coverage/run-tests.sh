#/bin/sh
# tools:
#   llvm-profdata: ?
#        llvm-cov: 15.0.7
#         genhtml: 1.16

# change to directory of the script
cd $(dirname "$0")

../../make.sh prof

RUN='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC.baz > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen; e=$?;if test $e -eq $EXP; then echo ok; else echo FAILED. expected $EXP got $e; exit 1; fi'
DIFF='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC.baz > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
DIFFINP='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC.baz > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./gen < ${SRC%.*}.in > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
DIFFPY='echo -n "$SRC: " && LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN $SRC.baz > gen.s && nasm -f elf64 gen.s && ld -s -o gen gen.o && ./${SRC%.*}.py > out; if cmp -s out ${SRC%.*}.out; then echo ok; else echo FAILED; exit 1; fi'
#BIN='valgrind --quiet --leak-check=full --show-leak-kinds=all -s ../../baz'
BIN='../../baz'

SRC=t1 && EXP=58 && eval $RUN
SRC=t2 && EXP=1 && eval $RUN
SRC=t3 && EXP=7 && eval $RUN
SRC=t4 && EXP=0 && eval $RUN
SRC=t5 && EXP=0 && eval $RUN
SRC=t6 && EXP=0 && eval $RUN
SRC=t7 && EXP=0 && eval $RUN
SRC=t8 && EXP=0 && eval $RUN
SRC=t9 && EXP=0 && eval $RUN
SRC=t10 && EXP=0 && eval $RUN
SRC=t11 && EXP=14 && eval $RUN
SRC=t12 && EXP=24 && eval $RUN
SRC=t13 && EXP=2 && eval $RUN
SRC=t14 && EXP=0 && eval $RUN
SRC=t15 && eval $DIFF
SRC=t16 && eval $DIFF
SRC=t17 && eval $DIFF
SRC=t18 && eval $DIFF
#SRC=t19 && EXP=120 && eval $RUN
SRC=t20 && EXP=7 && eval $RUN
SRC=t21 && EXP=12 && eval $RUN
SRC=t22 && EXP=16 && eval $RUN
SRC=t23 && EXP=17 && eval $RUN
SRC=t24 && EXP=7 && eval $RUN
SRC=t25 && EXP=0 && eval $RUN
SRC=t26 && EXP=0 && eval $RUN
SRC=t27 && EXP=0 && eval $RUN
SRC=t28 && EXP=0 && eval $RUN
SRC=t29 && EXP=0 && eval $RUN
SRC=t30 && EXP=0 && eval $RUN
SRC=t31 && EXP=0 && eval $RUN
SRC=t32 && EXP=0 && eval $RUN
SRC=t33 && EXP=0 && eval $RUN
SRC=t34 && EXP=0 && eval $RUN
SRC=t35 && EXP=0 && eval $RUN
SRC=t36 && eval $DIFFINP
SRC=t37 && EXP=0 && eval $RUN
SRC=t38 && EXP=0 && eval $RUN
SRC=t39 && EXP=0 && eval $RUN
SRC=t40 && EXP=0 && eval $RUN
SRC=t41 && EXP=0 && eval $RUN
SRC=t42 && EXP=0 && eval $RUN
SRC=t43 && EXP=0 && eval $RUN
SRC=t44 && EXP=0 && eval $RUN
SRC=t45 && EXP=0 && eval $RUN
SRC=t46 && EXP=0 && eval $RUN
SRC=t47 && EXP=0 && eval $RUN
SRC=t48 && EXP=0 && eval $RUN
SRC=t49 && EXP=0 && eval $RUN
SRC=t50 && EXP=0 && eval $RUN
SRC=t51 && EXP=0 && eval $RUN
SRC=t52 && EXP=0 && eval $RUN
SRC=t53 && EXP=0 && eval $RUN
SRC=t54 && EXP=0 && eval $RUN
SRC=t55 && EXP=0 && eval $RUN
SRC=t56 && EXP=0 && eval $RUN
SRC=t57 && EXP=0 && eval $RUN
SRC=t58 && EXP=0 && eval $RUN
SRC=t59 && EXP=0 && eval $RUN
SRC=t60 && EXP=0 && eval $RUN
SRC=t61 && EXP=0 && eval $RUN
SRC=t62 && EXP=0 && eval $RUN
SRC=t63 && eval $DIFFPY
SRC=t64 && EXP=0 && eval $RUN
SRC=t65 && EXP=0 && eval $RUN
SRC=t66 && EXP=0 && eval $RUN
SRC=t67 && EXP=0 && eval $RUN
SRC=t68 && EXP=0 && eval $RUN
SRC=t69 && EXP=0 && eval $RUN
SRC=t70 && EXP=0 && eval $RUN
SRC=t71 && EXP=0 && eval $RUN
SRC=t72 && EXP=0 && eval $RUN
SRC=t73 && EXP=5 && eval $RUN
SRC=t74 && EXP=0 && eval $RUN
SRC=t75 && EXP=0 && eval $RUN
SRC=t76 && EXP=0 && eval $RUN
SRC=t77 && EXP=0 && eval $RUN
SRC=t78 && EXP=0 && eval $RUN
SRC=t79 && EXP=0 && eval $RUN
SRC=t80 && EXP=0 && eval $RUN
SRC=t81 && EXP=0 && eval $RUN
SRC=t82 && EXP=0 && eval $RUN
SRC=t83 && EXP=0 && eval $RUN

rm gen gen.o gen.s diff.baz out

# process coverage data
llvm-profdata merge -o baz.profdata -sparse $(ls *.profraw) &&
  llvm-cov export --format=lcov --instr-profile baz.profdata --object $BIN >lcov.info &&
  # generate report
  genhtml --ignore-errors unsupported -quiet lcov.info --output-directory report/ &&
  echo &&
  echo coverage report generated in "$(realpath "report/")" &&
  echo &&
  rm *.profraw baz.profdata lcov.info
