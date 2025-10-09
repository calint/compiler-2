#/bin/sh
# tools:
#   llvm-profdata: ?
#        llvm-cov: 15.0.7
#         genhtml: 1.16
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd $SCRIPT_DIR

../../make.sh build prof asan

BIN='../../baz'

export UBSAN_OPTIONS="print_stacktrace=1"

export ASAN_OPTIONS="fast_unwind_on_fatal=0:print_stacktrace=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1:external_symbolizer=1"
export ASAN_SYMBOLIZER_PATH="$(which llvm-symbolizer)"
export ASAN_SYMBOLIZE=1

RUN() {
  echo -n "$SRC: "

  rm -f error.log

  LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN "$SRC.baz" 2>error.log >gen.s

  if [ $? -ne 0 ]; then
    echo "compiler failed. see 'error.log' and 'gen.s'" >&2
    exit 1
  fi

  nasm -f elf64 gen.s
  ld -s -o gen gen.o

  set +e
  ./gen
  e=$?
  set -e

  if test $e -eq $EXP; then
    echo ok
  else
    echo "FAILED. expected $EXP got $e"
    exit 1
  fi
}

DIFF() {
  echo -n "$SRC: "

  rm -f error.log

  LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN "$SRC.baz" 2>error.log >gen.s

  if [ $? -ne 0 ]; then
    echo "compiler failed. see 'error.log' and 'gen.s'" >&2
    exit 1
  fi

  nasm -f elf64 gen.s
  ld -s -o gen gen.o

  ./gen >out

  if cmp -s out "${SRC%.*}.out"; then
    echo ok
  else
    echo "FAILED. output differs. see: diff ${SRC%.*}.out out"
    exit 1
  fi
}

DIFFINP() {
  echo -n "$SRC: "

  rm -f error.log

  LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN "$SRC.baz" 2>error.log >gen.s

  if [ $? -ne 0 ]; then
    echo "compiler failed. see 'error.log' and 'gen.s'" >&2
    exit 1
  fi

  nasm -f elf64 gen.s
  ld -s -o gen gen.o

  ./gen <"${SRC%.*}.in" >out

  if cmp -s out "${SRC%.*}.out"; then
    echo ok
  else
    echo FAILED. output differs. see: diff ${SRC%.*}.out out
    exit 1
  fi
}

DIFFPY() {
  echo -n "$SRC: "

  rm -f error.log

  LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN "$SRC.baz" 2>error.log >gen.s

  nasm -f elf64 gen.s
  ld -s -o gen gen.o

  "./${SRC%.*}.py" >out

  if cmp -s out "${SRC%.*}.out"; then
    echo ok
  else
    echo FAILED. output differs. see: diff ${SRC%.*}.out out
    exit 1
  fi
}

source "$SCRIPT_DIR/run-tests-cases.sh"

rm gen gen.o gen.s diff.baz out error.log

# process coverage data
llvm-profdata merge -o baz.profdata -sparse $(ls *.profraw)
llvm-cov export --format=lcov --instr-profile baz.profdata --object $BIN >lcov.info
# generate report
genhtml --ignore-errors unsupported -quiet lcov.info --output-directory report/
echo
echo coverage report generated in "$(realpath "report/")"
echo
rm *.profraw baz.profdata lcov.info
